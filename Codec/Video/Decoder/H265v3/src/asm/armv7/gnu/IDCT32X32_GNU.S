@;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@void IDCT16X16(
@;							const short *pSrcData,
@;							const unsigned char *pPerdictionData,
@;							unsigned char *pDstRecoData,
@;							unsigned int uiDstStride)
@; short g_IDCT16X16H_EEE[8] = 
@;        {
@;				   64, 64, 83, 36,
@;           64,-64, 36,-83
@; 				};
@;
@; short g_IDCT16X16H_EO[16] = 
@;				{
@;					 89,  75,  50,  18,																																				
@;           75, -18, -89, -50,																																				
@;           50, -89,  18, +75 																																				
@;           18, -50,  75, -89
@;        }; 																																															  
@;@;@;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@;
        @AREA |.text|, CODE, READONLY    
        @IMPORT g_aIDCTCefT32_EEO
        @IMPORT g_aIDCTCefT32_EO
        @IMPORT g_aIDCTCefT32_O
        @EXPORT IDCT32X32_ASM
        @align 4
        #include     "h265dec_ASM_config.h"
        #include "../../../h265dec_ID.h"
        @#include "../../../h265dec_config.h"
        .section .text
        .align 4
        .if IDCT_ASM_ENABLED==1
        @.extern kg_IDCT_coef_for_t32_asm_EEO
        @.extern kg_IDCT_coef_for_t32_asm_EO
        @.extern kg_IDCT_coef_for_t32_asm_O
        .global IDCT32X32ASMV7
kg_IDCT_coef_for_t32_asm_EEO1:		
	.hword  89, 75, 50, 18
	.hword  75, -18, -89, -50
	.hword  50, -89, 18, 75
	.hword  18, -50, 75, -89 	
 .macro CMPUTE_O_K regdst
        pld     [r12]
        vld1.16  {d0, d1, d2, d3}, [r12]!   		@; g_aIDCTCefT32_O[k*16]~g_aIDCTCefT32_O[k*16+15]
        vmull.s16 \regdst, d4, d0[0]
        vmlal.s16 \regdst, d5, d0[1]
        vmlal.s16 \regdst, d6, d0[2]
        vmlal.s16 \regdst, d7, d0[3]
        vmlal.s16 \regdst, d8,  d1[0]
        vmlal.s16 \regdst, d9,  d1[1]
        vmlal.s16 \regdst, d10, d1[2]
        vmlal.s16 \regdst, d11, d1[3]
        vmlal.s16 \regdst, d12, d2[0]
        vmlal.s16 \regdst, d13, d2[1]
        vmlal.s16 \regdst, d14, d2[2]
        vmlal.s16 \regdst, d15, d2[3]
        vmlal.s16 \regdst, d16, d3[0]
        vmlal.s16 \regdst, d17, d3[1]
        vmlal.s16 \regdst, d18, d3[2]
        vmlal.s16 \regdst, d19, d3[3]

 .endm
        
IDCT32X32ASMV7:
        @; ldr  r12, = 2308
        stmfd sp!, {r4, r5, r6, r7,r8,r9, r10, r11, lr}
        @; 需要2048B放pTmpBlock[32*32]的值，但是存储顺序是使用vld4.16的，
        @; 再保存E[0]~E[15](0~3),需要4B*16*4=256
        @; 并保留空间为pTmpBlock是4字节对齐,故至少需要2308
        
       @;  sub  sp, sp, r12 			
       @;  and  r5, sp, #0xF
       @;  rsb  r5, r5, #0x10
       @;  add  r4, sp, r5     						@; E[0]~E[15]在sp中的地址
       @;  add  r5, r4, #256       				@; E[0]~E[15](0~3)之后存pTmpBlock
        
        mov  r5, r3 	;@pTmpBlock
        sub  sp, sp, #256 			;@ E[0]~E[15]在sp
        @;add  r4, sp, r5     						;@ E[0]~E[15]在sp中的地址
        mov  r4, sp     						;@ E[0]~E[15]在sp中的地址
        
        mov   r9, #8 										@; j=8
        
        mov  r11, r0
       
IDCT32X32_4Row_ASM_loop:      
        ldr  r12, = 0x240040
        mov  r14, #83
        vmov  d0, r12, r14      					@; d0[0~63]:64,36,83,0
        
        mov   r6, #256   			 				 	  @; pSrc[0]到pSrc[4*32]的跨度1 
        pld     [r0]
        ldr   r10, =-1664	 								@; pSrc[28*32]的地址到pSrc[2*32]的跨度  -256*7+128=-1664
        vld1.16  {d4},  [r0], r6 							;@pSrc[0*32]...[3]
        vld1.16  {d5},  [r0], r6 							;@pSrc[4*32]...[3]
        vld1.16  {d6},  [r0], r6 							;@pSrc[8*32]...[3]
        vld1.16  {d7},  [r0], r6 							;@pSrc[12*32]...[3]
        vld1.16  {d8},  [r0], r6 							;@pSrc[16*32]...[3]
        vld1.16  {d9},  [r0], r6 							;@pSrc[20*32]...[3]
        vld1.16  {d10}, [r0], r6 						  ;@pSrc[24*32]...[3]
        vld1.16  {d11}, [r0], r10 						;@pSrc[28*32]...[3] 
        
        vmull.s16 q6, d4, d0[0] 							@; 64 * pSrc[0]
        vmlal.s16 q6, d8, d0[0] 							@; +64 * pSrc[16*32]=EEEE[0]
        
        vmull.s16 q7, d4, d0[0] 							@; 64 * pSrc[0]
        vmlsl.s16 q7, d8, d0[0] 							@; -64 * pSrc[16*32]=EEEE[1]
        
        vmull.s16 q8, d6, d0[2] 							@; 83 * pSrc[8*32]
        vmlal.s16 q8, d10, d0[1] 							@; +36 * pSrc[24*32]=EEE0[0]
        
        vmull.s16 q9, d6, d0[1] 							@; 36 * pSrc[8*32]
        vmlsl.s16 q9, d10, d0[2] 							@; -83 * Src[24*32]=EEE0[1]
                                                                 
        vadd.s32  q12, q6, q8 								@; EEE[0] = EEEE[0] + EEEO[0];
        vadd.s32  q13, q7, q9 								@; EEE[1] = EEEE[1] + EEEO[1];
        vsub.s32  q14, q7, q9 								@; EEE[2] = EEEE[1] - EEEO[1];
        vsub.s32  q15, q6, q8 								@; EEE[3] = EEEE[0] - EEEO[0];
       
        adr   r12, kg_IDCT_coef_for_t32_asm_EEO1
        vld1.16  {d0, d1, d2, d3}, [r12]     @; g_aIDCTCefT32_EEO[0~15]
        
        vmull.s16 q6, d5, d0[0]
        vmlal.s16 q6, d7, d0[1]
        vmlal.s16 q6, d9, d0[2]
        vmlal.s16 q6, d11,d0[3] 							@; EEO[0]
        
        vmull.s16 q7, d5, d1[0]
        vmlal.s16 q7, d7, d1[1]
        vmlal.s16 q7, d9, d1[2]
        vmlal.s16 q7, d11,d1[3] 							@; EEO[1]
        
        vmull.s16 q8, d5, d2[0]
        vmlal.s16 q8, d7, d2[1]
        vmlal.s16 q8, d9, d2[2]
        vmlal.s16 q8, d11,d2[3] 							@; EEO[2]
        
        vmull.s16 q9, d5, d3[0]
        vmlal.s16 q9, d7, d3[1]
        vmlal.s16 q9, d9, d3[2]
        vmlal.s16 q9, d11,d3[3] 							@; EEO[3]
        
        vadd.s32  q10,q12, q6 								@; EE[0]
        vsub.s32  q6, q12, q6 								@; EE[7]
        vadd.s32  q11,q13, q7 								@; EE[1]
        vsub.s32  q7, q13, q7 								@; EE[6]        
        vadd.s32  q12,q14, q8 								@; EE[2]
        vsub.s32  q8, q14, q8 								@; EE[5]
        vadd.s32  q13,q15, q9 								@; EE[3] 
        vsub.s32  q9, q15, q9 								@; EE[4]
        
        pld     [r0] 
        ldr     r7,  = -1856                   @; pSrc[30*32]的地址到pSrc[1*32]的跨度 -256*7-32*2=-1856
        vld1.16  {d4},  [r0], r6 							;@pSrc[2*32]...[3]
        vld1.16  {d5},  [r0], r6 							;@pSrc[6*32]...[3]
        vld1.16  {d6},  [r0], r6 							;@pSrc[10*32]...[3]
        vld1.16  {d7},  [r0], r6 							;@pSrc[14*32]...[3]
        vld1.16  {d8},  [r0], r6 							;@pSrc[18*32]...[3]
        vld1.16  {d9},  [r0], r6 							;@pSrc[22*32]...[3]
        vld1.16  {d10}, [r0], r6 						  ;@pSrc[26*32]...[3]
        vld1.16  {d11}, [r0], r7 						;@pSrc[30*32]...[3] 
        
        adr  r12,  kg_IDCT_coef_for_t32_asm_EO   
        
        add  r14, r4, #224 									
        mov  r6, #-32
        @;=========== E[0], E[1], E[15],E[14]=============                         
        pld     [r12]
        vld1.16  {d0, d1, d2, d3}, [r12]!    @; g_aIDCTCefT32_EO[0~15]
        
        vmull.s16 q15, d4, d0[0]
        vmlal.s16 q15, d5, d0[1]
        vmlal.s16 q15, d6, d0[2]
        vmlal.s16 q15, d7, d0[3]
        vmlal.s16 q15, d8,  d1[0]
        vmlal.s16 q15, d9,  d1[1]
        vmlal.s16 q15, d10, d1[2]
        vmlal.s16 q15, d11, d1[3] 							@; EO[0]
        
        vadd.s32  q14, q10, q15  								@; E[0]=EE[0] + EO[0]
        vsub.s32  q10, q10, q15  								@; E[15]= EE[0] - EO[0]
        
        vmull.s16 q0, d4, d2[0]
        vmlal.s16 q0, d5, d2[1]
        vmlal.s16 q0, d6, d2[2]
        vmlal.s16 q0, d7, d2[3]
        vmlal.s16 q0, d8,  d3[0]
        vmlal.s16 q0, d9,  d3[1]
        vmlal.s16 q0, d10, d3[2]
        vmlal.s16 q0, d11, d3[3] 							@; EO[1]
        
        vadd.s32  q15, q11, q0  								@; E[1]=EE[1] + EO[1]
        vsub.s32  q11, q11, q0  								@; E[14]= EE[1] - EO[1]
        vswp   q10, q11
        vst1.32 {d28,d29,d30,d31}, [r4]!  			@; E[0], E[1]
        vst1.32 {d20,d21,d22,d23}, [r14], r6		@; E[14],E[15]
        
        @;=========== E[2], E[3], E[13],E[12]=============
        vld1.16  {d0, d1, d2, d3}, [r12]!    @; g_aIDCTCefT32_EO[16~31]
        
        vmull.s16 q15, d4, d0[0]
        vmlal.s16 q15, d5, d0[1]
        vmlal.s16 q15, d6, d0[2]
        vmlal.s16 q15, d7, d0[3]
        vmlal.s16 q15, d8,  d1[0]
        vmlal.s16 q15, d9,  d1[1]
        vmlal.s16 q15, d10, d1[2]
        vmlal.s16 q15, d11, d1[3] 							@; EO[2]
        
        vadd.s32  q14, q12, q15  								@; E[2]=EE[2] + EO[2]
        vsub.s32  q11, q12, q15  								@; E[13]= EE[2] - EO[2]
        
        vmull.s16 q0, d4, d2[0]
        vmlal.s16 q0, d5, d2[1]
        vmlal.s16 q0, d6, d2[2]
        vmlal.s16 q0, d7, d2[3]
        vmlal.s16 q0, d8,  d3[0]
        vmlal.s16 q0, d9,  d3[1]
        vmlal.s16 q0, d10, d3[2]
        vmlal.s16 q0, d11, d3[3] 							@; EO[3]
        
        vadd.s32  q15, q13, q0  								@; E[3]=EE[3] + EO[3]
        vsub.s32  q10, q13, q0  								@; E[12]= EE[3] - EO[3]
        
        vst1.32 {d28,d29,d30,d31}, [r4]!  			@; E[2], E[3]
        vst1.32 {d20,d21,d22,d23}, [r14], r6			@; E[12],E[13]
         
        @;=========== E[4], E[5], E[11],E[10]=============
        vld1.16  {d0, d1, d2, d3}, [r12]!    @; g_aIDCTCefT32_EO[32~47]
        
        vmull.s16 q15, d4, d0[0]
        vmlal.s16 q15, d5, d0[1]
        vmlal.s16 q15, d6, d0[2]
        vmlal.s16 q15, d7, d0[3]
        vmlal.s16 q15, d8,  d1[0]
        vmlal.s16 q15, d9,  d1[1]
        vmlal.s16 q15, d10, d1[2]
        vmlal.s16 q15, d11, d1[3] 							@; EO[4]
        
        vadd.s32  q14, q9, q15  								@; E[4]=EE[4] + EO[4]
        vsub.s32  q11, q9, q15  								@; E[11]= EE[4] - EO[4]
        
        vmull.s16 q0, d4, d2[0]
        vmlal.s16 q0, d5, d2[1]
        vmlal.s16 q0, d6, d2[2]
        vmlal.s16 q0, d7, d2[3]
        vmlal.s16 q0, d8,  d3[0]
        vmlal.s16 q0, d9,  d3[1]
        vmlal.s16 q0, d10, d3[2]
        vmlal.s16 q0, d11, d3[3] 							@; EO[5]
        
        vadd.s32  q15, q8, q0  								@; E[5]=EE[5] + EO[5]
        vsub.s32  q10, q8, q0  								@; E[10]= EE[5] - EO[5]
        
        vst1.32 {d28,d29,d30,d31}, [r4]!  			@; E[4], E[5]
        vst1.32 {d20,d21,d22,d23}, [r14], r6		@; E[10],E[11],
        
        @;=========== E[6], E[7], E[9],E[8]=============
        vld1.16  {d0, d1, d2, d3}, [r12]!    @; g_aIDCTCefT32_EO[48~63]
        
        vmull.s16 q15, d4, d0[0]
        vmlal.s16 q15, d5, d0[1]
        vmlal.s16 q15, d6, d0[2]
        vmlal.s16 q15, d7, d0[3]
        vmlal.s16 q15, d8,  d1[0]
        vmlal.s16 q15, d9,  d1[1]
        vmlal.s16 q15, d10, d1[2]
        vmlal.s16 q15, d11, d1[3] 							@; EO[6]
        
        vadd.s32  q14, q7, q15  								@; E[6]=EE[6] + EO[6]
        vsub.s32  q11, q7, q15  								@; E[9]= EE[6] - EO[6]
        
        vmull.s16 q0, d4, d2[0]
        vmlal.s16 q0, d5, d2[1]
        vmlal.s16 q0, d6, d2[2]
        vmlal.s16 q0, d7, d2[3]
        vmlal.s16 q0, d8,  d3[0]
        vmlal.s16 q0, d9,  d3[1]
        vmlal.s16 q0, d10, d3[2]
        vmlal.s16 q0, d11, d3[3] 							@; EO[7]
        
        vadd.s32  q15, q6, q0  								@; E[7]=EE[7] + EO[7]
        vsub.s32  q10, q6, q0  								@; E[8]= EE[7] - EO[7]
        
        vst1.32 {d28,d29,d30,d31}, [r4]!  			@; E[6], E[7]
        vst1.32 {d20,d21,d22,d23}, [r4] 			@; E[8],E[9],
        sub   r4, r4, #128
        @; =======pSrc[k*32],k=1,3,5...,31==========
        adr   r12, kg_IDCT_coef_for_t32_asm_O
        mov  r7, #128
        ldr  r8, = -1976      @; 从pSrc[31*32]到pSrc[0*32][4]的距离：-128*15-32*2+4*2=-1976
        pld     [r0]
        vld1.16  {d4},  [r0], r7 							;@pSrc[1	*	32]...[3]
        vld1.16  {d5},  [r0], r7 							;@pSrc[3	*	32]...[3]
        vld1.16  {d6},  [r0], r7 							;@pSrc[5	*	32]...[3]
        vld1.16  {d7},  [r0], r7 							;@pSrc[7	*	32]...[3]
        vld1.16  {d8},  [r0], r7 							;@pSrc[9	*	32]...[3]
        vld1.16  {d9},  [r0], r7 							;@pSrc[11	*	32]...[3]
        vld1.16  {d10}, [r0], r7 						  ;@pSrc[13	*	32]...[3]
        vld1.16  {d11}, [r0], r7 					  	;@pSrc[15	*	32]...[3]
        vld1.16  {d12}, [r0], r7 							;@pSrc[17	*	32]...[3]
        vld1.16  {d13}, [r0], r7 							;@pSrc[19	*	32]...[3]
        vld1.16  {d14}, [r0], r7 							;@pSrc[21	*	32]...[3]
        vld1.16  {d15}, [r0], r7 							;@pSrc[23	*	32]...[3]
        vld1.16  {d16}, [r0], r7 							;@pSrc[25	*	32]...[3]
        vld1.16  {d17}, [r0], r7 							;@pSrc[27	*	32]...[3]
        vld1.16  {d18}, [r0], r7 						  ;@pSrc[29	*	32]...[3]
        vld1.16  {d19}, [r0], r8  						;@pSrc[31	*	32]...[3]
        @; 此时r0指向pSrc[0*32][4]
        @; =======O[k]==========
        add   r7, r5, #224 										@; pTmpBlock+31的地址
        mov   r8, #-32
        mov   r14, #4 												@; 控制计算O[k]
        ;@r4为E[0]的地址
		
IDCT32X32_4Row_4O:        
        CMPUTE_O_K q10 												@; O[0]
        vld1.32   {d28,d29,d30,d31}, [r4]! 		@; E[0],E[1]
        vadd.s32  q12, q14, q10 							@; E[0] + O[0]
        vsub.s32  q10, q14, q10 							@; E[0] - O[0]
        vqrshrn.s32 d24, q12, #7 								@; pTmpBlock[00~03]
        vqrshrn.s32 d20, q10, #7 								@; pTmpBlock[310~313]
         
        CMPUTE_O_K q11 												@; O[1]
        vadd.s32  q13, q15, q11 							@; E[1] + O[1]
        vsub.s32  q11, q15, q11 							@; E[1] - O[1]
        vqrshrn.s32 d25, q13, #7 								@; pTmpBlock[10~13]
        vqrshrn.s32 d21, q11, #7 								@; pTmpBlock[300~303]
        
        CMPUTE_O_K q11 												@; O[2]
        vld1.32   {d28,d29,d30,d31}, [r4]! 		@; E[2],E[3]
        vadd.s32  q13, q14, q11 							@; E[2] + O[2]
        vsub.s32  q11, q14, q11 							@; E[2] - O[2]
        vqrshrn.s32 d26, q13, #7 								@; pTmpBlock[20~23]
        vqrshrn.s32 d22, q11, #7 								@; pTmpBlock[290~293]
         
        CMPUTE_O_K q14 												@; O[1]
        vadd.s32  q0, q15, q14 							@; E[1] + O[1]
        vsub.s32  q15, q15, q14 							@; E[1] - O[1]
        vqrshrn.s32 d27, q0, #7 								@; pTmpBlock[10~13]
        vqrshrn.s32 d23, q15, #7 								@; pTmpBlock[300~303]
        @; 为了转置存储0~3行和28~31行的4个元素,需要将31~28的寄存器顺序排列
        vswp   d20, d21
        vswp   d22, d23
        vswp   q10, q11
        @; d24~d27存放的是00~03,10~13,20~23,30~33
        @; d20~d23存放的是280~283,290~293,300~303,310~313        
        vst4.16 {d24,d25,d26,d27}, [r5]!
        vst4.16 {d20,d21,d22,d23}, [r7], r8
        
        subs    r14, r14, #1
        bgt    IDCT32X32_4Row_4O
        sub     r4, r4, #256 								@; 回到存放E[0]的sp地址
        add     r5, r5, #128 								@; 下一次循环需要使用sp存放pTmpBlock的地址
        subs    r9, r9, #1 						@; j--
        bgt     IDCT32X32_4Row_ASM_loop
		b		PIC_label1
kg_IDCT_coef_for_t32_asm_EEO:		
	.hword  89, 75, 50, 18
	.hword  75, -18, -89, -50
	.hword  50, -89, 18, 75
	.hword  18, -50, 75, -89 		
kg_IDCT_coef_for_t32_asm_EO:
	.hword  90, 87, 80, 70, 57, 43, 25, 9
	.hword  87, 57, 9, -43, -80, -90, -70, -25
	.hword  80, 9, -70, -87, -25, 57, 90, 43
	.hword  70, -43, -87, 9, 90, 25, -80, -57
	.hword  57, -80, -25, 90, -9, -87, 43, 70
	.hword  43, -90, 57, 25, -87, 70, 9, -80
	.hword  25, -70, 90, -80, 43, 9, -57, 87
	.hword  9, -25, 43, -57, 70, -80, 87, -90
kg_IDCT_coef_for_t32_asm_O:
	.hword    90, 90, 88, 85, 82, 78, 73, 67, 61, 54, 46, 38, 31, 22, 13, 4
	.hword    90, 82, 67, 46, 22, -4, -31, -54, -73, -85, -90, -88, -78, -61, -38, -13
	.hword    88, 67, 31, -13, -54, -82, -90, -78, -46, -4, 38, 73, 90, 85, 61, 22
	.hword    85, 46, -13, -67, -90, -73, -22, 38, 82, 88, 54, -4, -61, -90, -78, -31
	.hword    82, 22, -54, -90, -61, 13, 78, 85, 31, -46, -90, -67, 4, 73, 88, 38
	.hword    78, -4, -82, -73, 13, 85, 67, -22, -88, -61, 31, 90, 54, -38, -90, -46
	.hword    73, -31, -90, -22, 78, 67, -38, -90, -13, 82, 61, -46, -88, -4, 85, 54
	.hword    67, -54, -78, 38, 85, -22, -90, 4, 90, 13, -88, -31, 82, 46, -73, -61
	.hword    61, -73, -46, 82, 31, -88, -13, 90, -4, -90, 22, 85, -38, -78, 54, 67
	.hword    54, -85, -4, 88, -46, -61, 82, 13, -90, 38, 67, -78, -22, 90, -31, -73
	.hword    46, -90, 38, 54, -90, 31, 61, -88, 22, 67, -85, 13, 73, -82, 4, 78
	.hword    38, -88, 73, -4, -67, 90, -46, -31, 85, -78, 13, 61, -90, 54, 22, -82
	.hword    31, -78, 90, -61, 4, 54, -88, 82, -38, -22, 73, -90, 67, -13, -46, 85
	.hword    22, -61, 85, -90, 73, -38, -4, 46, -78, 90, -82, 54, -13, -31, 67, -88
	.hword    13, -38, 61, -78, 88, -90, 85, -73, 54, -31, 4, 22, -46, 67, -82, 90
	.hword    4, -13, 22, -31, 38, -46, 54, -61, 67, -73, 78, -82, 85, -88, 90, -90
	
        @; ===============第二次变换==================
        @;@sub    r5, r5, #2048  			@; 回到pTmpBlock[0]的位置--与下语句的r0是一致的
        @ ;@add    r0, r4, #256 				;@ 下一次循环是数据源地址，即上一次pTmpBlock的地址
PIC_label1:		
        sub    r0, r5, #2048 				;@ 下一次循环是数据源地址，即上一次pTmpBlock的地址
        mov    r5, r11 							@; 存储在原始pSrc的内存中
        mov    r9, #8 							@; j=8
        
IDCT32X32_4Row_ASM_loop_2:    
        ldr  r12, = 0x240040
        mov  r14, #83
        vmov  d0, r12, r14      					@; d0[0~63]:64,36,83,0
        
        mov   r6, #256   			 				 	  @; pSrc[0]到pSrc[4*32]的跨度1 
        ldr   r10, =-1776	 								@; pSrc[28*32]的地址到pSrc[2*32]的跨度  -256*7+8*2=-1776
        pld     [r0]
        vld1.16  {d4},  [r0], r6 							;@pSrc[0*32]...[3]
        vld1.16  {d5},  [r0], r6 							;@pSrc[4*32]...[3]
        vld1.16  {d6},  [r0], r6 							;@pSrc[8*32]...[3]
        vld1.16  {d7},  [r0], r6 							;@pSrc[12*32]...[3]
        vld1.16  {d8},  [r0], r6 							;@pSrc[16*32]...[3]
        vld1.16  {d9},  [r0], r6 							;@pSrc[20*32]...[3]
        vld1.16  {d10}, [r0], r6 						  ;@pSrc[24*32]...[3]
        vld1.16  {d11}, [r0], r10 						;@pSrc[28*32]...[3] 
        
        vmull.s16 q6, d4, d0[0] 							@; 64 * pSrc[0]
        vmlal.s16 q6, d8, d0[0] 							@; +64 * pSrc[16*32]=EEEE[0]
        
        vmull.s16 q7, d4, d0[0] 							@; 64 * pSrc[0]
        vmlsl.s16 q7, d8, d0[0] 							@; -64 * pSrc[16*32]=EEEE[1]
        
        vmull.s16 q8, d6, d0[2] 							@; 83 * pSrc[8*32]
        vmlal.s16 q8, d10, d0[1] 							@; +36 * pSrc[24*32]=EEE0[0]
        
        vmull.s16 q9, d6, d0[1] 							@; 36 * pSrc[8*32]
        vmlsl.s16 q9, d10, d0[2] 							@; -83 * Src[24*32]=EEE0[1]
                                                                 
        vadd.s32  q12, q6, q8 								@; EEE[0] = EEEE[0] + EEEO[0];
        vadd.s32  q13, q7, q9 								@; EEE[1] = EEEE[1] + EEEO[1];
        vsub.s32  q14, q7, q9 								@; EEE[2] = EEEE[1] - EEEO[1];
        vsub.s32  q15, q6, q8 								@; EEE[3] = EEEE[0] - EEEO[0];
        
        adr   r12, kg_IDCT_coef_for_t32_asm_EEO
        vld1.16  {d0, d1, d2, d3}, [r12]     @; g_aIDCTCefT32_EEO[0~15]
        
        vmull.s16 q6, d5, d0[0]
        vmlal.s16 q6, d7, d0[1]
        vmlal.s16 q6, d9, d0[2]
        vmlal.s16 q6, d11,d0[3] 							@; EEO[0]
        
        vmull.s16 q7, d5, d1[0]
        vmlal.s16 q7, d7, d1[1]
        vmlal.s16 q7, d9, d1[2]
        vmlal.s16 q7, d11,d1[3] 							@; EEO[1]
        
        vmull.s16 q8, d5, d2[0]
        vmlal.s16 q8, d7, d2[1]
        vmlal.s16 q8, d9, d2[2]
        vmlal.s16 q8, d11,d2[3] 							@; EEO[2]
        
        vmull.s16 q9, d5, d3[0]
        vmlal.s16 q9, d7, d3[1]
        vmlal.s16 q9, d9, d3[2]
        vmlal.s16 q9, d11,d3[3] 							@; EEO[3]
        
        vadd.s32  q10,q12, q6 								@; EE[0]
        vsub.s32  q6, q12, q6 								@; EE[7]
        vadd.s32  q11,q13, q7 								@; EE[1]
        vsub.s32  q7, q13, q7 								@; EE[6]        
        vadd.s32  q12,q14, q8 								@; EE[2]
        vsub.s32  q8, q14, q8 								@; EE[5]
        vadd.s32  q13,q15, q9 								@; EE[3] 
        vsub.s32  q9, q15, q9 								@; EE[4]

        ldr     r7, = -1800                  @; pSrc[30*32]的地址到pSrc[1*32]的跨度 -256*7-4*2=-1800
        pld     [r0]
        vld1.16  {d4},  [r0], r6 							;@pSrc[2*32]...[3]
        vld1.16  {d5},  [r0], r6 							;@pSrc[6*32]...[3]
        vld1.16  {d6},  [r0], r6 							;@pSrc[10*32]...[3]
        vld1.16  {d7},  [r0], r6 							;@pSrc[14*32]...[3]
        vld1.16  {d8},  [r0], r6 							;@pSrc[18*32]...[3]
        vld1.16  {d9},  [r0], r6 							;@pSrc[22*32]...[3]
        vld1.16  {d10}, [r0], r6 						  ;@pSrc[26*32]...[3]
        vld1.16  {d11}, [r0], r7 						;@pSrc[30*32]...[3] 
        
        adr  r12,  kg_IDCT_coef_for_t32_asm_EO   
        
        add  r14, r4, #224   									
        mov  r6, #-32
        @;=========== E[0], E[1], E[15],E[14]=============                         
        vld1.16  {d0, d1, d2, d3}, [r12]!    @; g_aIDCTCefT32_EO[0~15]
        
        vmull.s16 q15, d4, d0[0]
        vmlal.s16 q15, d5, d0[1]
        vmlal.s16 q15, d6, d0[2]
        vmlal.s16 q15, d7, d0[3]
        vmlal.s16 q15, d8,  d1[0]
        vmlal.s16 q15, d9,  d1[1]
        vmlal.s16 q15, d10, d1[2]
        vmlal.s16 q15, d11, d1[3] 							@; EO[0]
        
        vadd.s32  q14, q10, q15  								@; E[0]=EE[0] + EO[0]
        vsub.s32  q10, q10, q15  								@; E[15]= EE[0] - EO[0]
        
        vmull.s16 q0, d4, d2[0]
        vmlal.s16 q0, d5, d2[1]
        vmlal.s16 q0, d6, d2[2]
        vmlal.s16 q0, d7, d2[3]
        vmlal.s16 q0, d8,  d3[0]
        vmlal.s16 q0, d9,  d3[1]
        vmlal.s16 q0, d10, d3[2]
        vmlal.s16 q0, d11, d3[3] 							@; EO[1]
        
        vadd.s32  q15, q11, q0  								@; E[1]=EE[1] + EO[1]
        vsub.s32  q11, q11, q0  								@; E[14]= EE[1] - EO[1]
        vswp   q10, q11
        vst1.32 {d28,d29,d30,d31}, [r4]!  			@; E[0], E[1]
        vst1.32 {d20,d21,d22,d23}, [r14], r6		@; E[14],E[15]
        
        @;=========== E[2], E[3], E[13],E[12]=============
        vld1.16  {d0, d1, d2, d3}, [r12]!    @; g_aIDCTCefT32_EO[16~31]
        
        vmull.s16 q15, d4, d0[0]
        vmlal.s16 q15, d5, d0[1]
        vmlal.s16 q15, d6, d0[2]
        vmlal.s16 q15, d7, d0[3]
        vmlal.s16 q15, d8,  d1[0]
        vmlal.s16 q15, d9,  d1[1]
        vmlal.s16 q15, d10, d1[2]
        vmlal.s16 q15, d11, d1[3] 							@; EO[2]
        
        vadd.s32  q14, q12, q15  								@; E[2]=EE[2] + EO[2]
        vsub.s32  q11, q12, q15  								@; E[13]= EE[2] - EO[2]
        
        vmull.s16 q0, d4, d2[0]
        vmlal.s16 q0, d5, d2[1]
        vmlal.s16 q0, d6, d2[2]
        vmlal.s16 q0, d7, d2[3]
        vmlal.s16 q0, d8,  d3[0]
        vmlal.s16 q0, d9,  d3[1]
        vmlal.s16 q0, d10, d3[2]
        vmlal.s16 q0, d11, d3[3] 							@; EO[3]
        
        
        vadd.s32  q15, q13, q0  								@; E[3]=EE[3] + EO[3]
        vsub.s32  q10, q13, q0  								@; E[12]= EE[3] - EO[3]
        
        vst1.32 {d28,d29,d30,d31}, [r4]!  			@; E[2], E[3]
        vst1.32 {d20,d21,d22,d23}, [r14], r6			@; E[12],E[13]
         
        @;=========== E[4], E[5], E[11],E[10]=============
        vld1.16  {d0, d1, d2, d3}, [r12]!    @; g_aIDCTCefT32_EO[32~47]
        
        vmull.s16 q15, d4, d0[0]
        vmlal.s16 q15, d5, d0[1]
        vmlal.s16 q15, d6, d0[2]
        vmlal.s16 q15, d7, d0[3]
        vmlal.s16 q15, d8,  d1[0]
        vmlal.s16 q15, d9,  d1[1]
        vmlal.s16 q15, d10, d1[2]
        vmlal.s16 q15, d11, d1[3] 							@; EO[4]
        
        vadd.s32  q14, q9, q15  								@; E[4]=EE[4] + EO[4]
        vsub.s32  q11, q9, q15  								@; E[11]= EE[4] - EO[4]
        
        vmull.s16 q0, d4, d2[0]
        vmlal.s16 q0, d5, d2[1]
        vmlal.s16 q0, d6, d2[2]
        vmlal.s16 q0, d7, d2[3]
        vmlal.s16 q0, d8,  d3[0]
        vmlal.s16 q0, d9,  d3[1]
        vmlal.s16 q0, d10, d3[2]
        vmlal.s16 q0, d11, d3[3] 							@; EO[5]
        
        vadd.s32  q15, q8, q0  								@; E[5]=EE[5] + EO[5]
        vsub.s32  q10, q8, q0  								@; E[10]= EE[5] - EO[5]
        
        vst1.32 {d28,d29,d30,d31}, [r4]!  			@; E[4], E[5]
        vst1.32 {d20,d21,d22,d23}, [r14], r6		@; E[10],E[11],
        
        @;=========== E[6], E[7], E[9],E[8]=============
        vld1.16  {d0, d1, d2, d3}, [r12]!    @; g_aIDCTCefT32_EO[48~63]
        
        vmull.s16 q15, d4, d0[0]
        vmlal.s16 q15, d5, d0[1]
        vmlal.s16 q15, d6, d0[2]
        vmlal.s16 q15, d7, d0[3]
        vmlal.s16 q15, d8,  d1[0]
        vmlal.s16 q15, d9,  d1[1]
        vmlal.s16 q15, d10, d1[2]
        vmlal.s16 q15, d11, d1[3] 							@; EO[6]
        
        vadd.s32  q14, q7, q15  								@; E[6]=EE[6] + EO[6]
        vsub.s32  q11, q7, q15  								@; E[9]= EE[6] - EO[6]
        
        vmull.s16 q0, d4, d2[0]
        vmlal.s16 q0, d5, d2[1]
        vmlal.s16 q0, d6, d2[2]
        vmlal.s16 q0, d7, d2[3]
        vmlal.s16 q0, d8,  d3[0]
        vmlal.s16 q0, d9,  d3[1]
        vmlal.s16 q0, d10, d3[2]
        vmlal.s16 q0, d11, d3[3] 							@; EO[7]
        
        vadd.s32  q15, q6, q0  								@; E[7]=EE[7] + EO[7]
        vsub.s32  q10, q6, q0  								@; E[8]= EE[7] - EO[7]
        
        vst1.32 {d28,d29,d30,d31}, [r4]!  			@; E[6], E[7]
        vst1.32 {d20,d21,d22,d23}, [r4] 			@; E[8],E[9],
        sub   r4, r4, #128
        @; =======pSrc[k*32],k=1,3,5...,31==========
        adr   r12, kg_IDCT_coef_for_t32_asm_O
        mov  r7,  #16
        mov  r14, #240
        ldr  r8, = -1784      @; 从pSrc[31*32]到pSrc[0*32][4]的距离：-16*8-240*7+12*2B=
        pld     [r0]
        vld1.16  {d4},  [r0], r7 							;@pSrc[1	*	32]...[3]
        vld1.16  {d5},  [r0], r14 						;@pSrc[3	*	32]...[3]
        vld1.16  {d6},  [r0], r7 							;@pSrc[5	*	32]...[3]
        vld1.16  {d7},  [r0], r14 						;@pSrc[7	*	32]...[3]
        vld1.16  {d8},  [r0], r7 							;@pSrc[9	*	32]...[3]
        vld1.16  {d9},  [r0], r14 						;@pSrc[11	*	32]...[3]
        vld1.16  {d10}, [r0], r7 						  ;@pSrc[13	*	32]...[3]
        vld1.16  {d11}, [r0], r14 				  	;@pSrc[15	*	32]...[3]
        vld1.16  {d12}, [r0], r7 							;@pSrc[17	*	32]...[3]
        vld1.16  {d13}, [r0], r14 						;@pSrc[19	*	32]...[3]
        vld1.16  {d14}, [r0], r7 							;@pSrc[21	*	32]...[3]
        vld1.16  {d15}, [r0], r14 						;@pSrc[23	*	32]...[3]
        vld1.16  {d16}, [r0], r7 							;@pSrc[25	*	32]...[3]
        vld1.16  {d17}, [r0], r14 						;@pSrc[27	*	32]...[3]
        vld1.16  {d18}, [r0], r7 						  ;@pSrc[29	*	32]...[3]
        vld1.16  {d19}, [r0], r8  						;@pSrc[31	*	32]...[3]
        @; 此时r0指向pSrc[0*32][4]
        @; =======O[k]==========
        add   r7, r5, #224 										@; pTmpBlock+31的地址
        mov   r8, #-32
        mov   r14, #4 												@; 控制计算O[k]
        ;@r4为E[0]的地址
IDCT32X32_4Row_4O_2:
        CMPUTE_O_K q10 												@; O[0]
        vld1.32   {d28,d29,d30,d31}, [r4]! 		@; E[0],E[1]
        vadd.s32  q12, q14, q10 							@; E[0] + O[0]
        vsub.s32  q10, q14, q10 							@; E[0] - O[0]
        vqrshrn.s32 d24, q12, #12 								@; pTmpBlock[00~03]
        vqrshrn.s32 d20, q10, #12 								@; pTmpBlock[310~313]
         
        CMPUTE_O_K q11 												@; O[1]
        vadd.s32  q13, q15, q11 							@; E[1] + O[1]
        vsub.s32  q11, q15, q11 							@; E[1] - O[1]
        vqrshrn.s32 d25, q13, #12 								@; pTmpBlock[10~13]
        vqrshrn.s32 d21, q11, #12 								@; pTmpBlock[300~303]
        
        CMPUTE_O_K q11 												@; O[2]
        vld1.32   {d28,d29,d30,d31}, [r4]! 		@; E[2],E[3]
        vadd.s32  q13, q14, q11 							@; E[2] + O[2]
        vsub.s32  q11, q14, q11 							@; E[2] - O[2]
        vqrshrn.s32 d26, q13, #12 								@; pTmpBlock[20~23]
        vqrshrn.s32 d22, q11, #12 								@; pTmpBlock[290~293]
         
        CMPUTE_O_K q14 												@; O[1]
        vadd.s32  q0, q15, q14 							@; E[1] + O[1]
        vsub.s32  q15, q15, q14 							@; E[1] - O[1]
        vqrshrn.s32 d27, q0, #12 								@; pTmpBlock[10~13]
        vqrshrn.s32 d23, q15, #12 								@; pTmpBlock[300~303]
        @; 为了转置存储0~3行和28~31行的4个元素,需要将31~28的寄存器顺序排列
        vswp   d20, d21
        vswp   d22, d23
        vswp   q10, q11
        @; d24~d27存放的是00~03,10~13,20~23,30~33
        @; d20~d23存放的是280~283,290~293,300~303,310~313        
        vst4.16 {d24,d25,d26,d27}, [r5]!
        vst4.16 {d20,d21,d22,d23}, [r7], r8
        
        subs    r14, r14, #1
        bgt    IDCT32X32_4Row_4O_2
        sub     r4, r4, #256 								@; 回到存放E[0]的sp地址
        add     r5, r5, #128 								@; 下一次循环需要使用sp存放pTmpBlock的地址
        subs    r9, r9, #1 						@; j--
        bgt     IDCT32X32_4Row_ASM_loop_2
        
        @; 恢复pTmpBlock的首地址应该和r11相等,即残差的地址
        sub  r5, r5, #2048
        add   sp, sp, #256									
        mov  r6, #32 							@; 跨度
        @;mov  r7, #PRED_CACHE_STRIDE 
        mov  r7, r1	 			@; pred_stride
        mov  r9, #31
        mov  r8, #-216
        mov  r4, #4
IDCT32X32_ASM_RESI_ADD_PRED:   
        pld     [r5]
        vld1.16  {d0},  [r5], r6 							;@残差[01]...[03]
        vld1.16  {d1},  [r5], r6 							;@残差[04]...[07]
        vld1.16  {d2},  [r5], r6 							;@残差[08]...[011]
        vld1.16  {d3},  [r5], r6 							;@残差[012]...[015]
        vld1.16  {d4},  [r5], r6 							;@残差[016]...[019]
        vld1.16  {d5},  [r5], r6 							;@残差[020]...[023]
        vld1.16  {d6},  [r5], r6 							;@残差[024]...[027]
        vld1.16  {d7},  [r5], r8 							;@残差[028]...[031]
        @; 此时r5指向a10
        pld     [r1]
        vld1.16  {d8,d9,d10,d11}, [r1],r2 		@; pPerdiction[0]...[31]
        
        vaddw.u8 q6, q0, d8
        vaddw.u8 q7, q1, d9
        vaddw.u8 q8, q2, d10
        vaddw.u8 q9, q3, d11
        
        vqmovun.s16 d20, q6
        vqmovun.s16 d21, q7
        vqmovun.s16 d22, q8
        vqmovun.s16 d23, q9
        
        vst1.8  {d20,d21,d22,d23}, [r7], r2
        ands   r12, r9, #3
        addeq  r5, r5, #224
        
        subs   r9, r9, #1
        bge    IDCT32X32_ASM_RESI_ADD_PRED
        
        ldr r12, = 2308
        @;add   sp, sp, #256
        ldmfd sp!, {r4, r5, r6, r7,r8,r9, r10, r11, pc}
        @end
        .endif    @.if IDCT_ASM_ENABLED==1
        .end
        