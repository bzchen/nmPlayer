/*-------------------------------------------------------------------*
 *                         QISF_NS.C								 *
 *-------------------------------------------------------------------*
 *                                                                   *
 * Coding/Decoding of ISF parameters for background noise.           *
 *                                                                   *
 * The ISF vector is quantized using VQ with split-by-5              *
 *-------------------------------------------------------------------*/

#include "typedef.h"
#include "basic_op.h"
#include "acelp_fx.h"
#include "count.h"


#define ORDER   16            /* order of linear prediction filter */
#define ISF_GAP 128

#define SIZE_BK_NOISE1  64
#define SIZE_BK_NOISE2  64
#define SIZE_BK_NOISE3  64
#define SIZE_BK_NOISE4  32
#define SIZE_BK_NOISE5  32


#include "qisf_ns.tab"                     /* Codebooks of ISFs */

/*------------------------------------------------------------------*
 * routine:   Qisf_ns()                                             *
 *            ~~~~~~~~~                                             *
 *------------------------------------------------------------------*/

void voAMRWBPDecQisf_ns(
     Word16 * isf1,                        /* input : ISF in the frequency domain (0..0.5) */
     Word16 * isf_q,                       /* output: quantized ISF                        */
     Word16 * indice                       /* output: quantization indices                 */
)
{
    Word16 i;
    Word32 tmp;

    for (i = 0; i < ORDER; i++)
    {
        isf_q[i] = sub(isf1[i], mean_isf_noise[i]);     
    }

    indice[0] = voAMRWBPDecSubVQ(&isf_q[0], dico1_isf_noise, 2, SIZE_BK_NOISE1, &tmp);    
    indice[1] = voAMRWBPDecSubVQ(&isf_q[2], dico2_isf_noise, 3, SIZE_BK_NOISE2, &tmp);    
    indice[2] = voAMRWBPDecSubVQ(&isf_q[5], dico3_isf_noise, 3, SIZE_BK_NOISE3, &tmp);    
    indice[3] = voAMRWBPDecSubVQ(&isf_q[8], dico4_isf_noise, 4, SIZE_BK_NOISE4, &tmp);    
    indice[4] = voAMRWBPDecSubVQ(&isf_q[12], dico5_isf_noise, 4, SIZE_BK_NOISE5, &tmp);   

    /* decoding the ISFs */

    voAMRWBPDecDisf_ns(indice, isf_q);

    return;
}

/*-------------------------------------------------------------------*
 * routine:   Disf_ns()                                              *
 *            ~~~~~~~~~                                              *
 * Decoding of ISF parameters                                        *
 *-------------------------------------------------------------------*
 *  Arguments:                                                       *
 *    indice[] : indices of the selected codebook entries            *
 *    isf[]    : quantized ISFs (in frequency domain)                *
 *-------------------------------------------------------------------*/

void voAMRWBPDecDisf_ns(
     Word16 * indice,                      /* input:  quantization indices                  */
     Word16 * isf_q                        /* input : ISF in the frequency domain (0..0.5)  */
)
{
    Word16 i;

    for (i = 0; i < 2; i++)
    {
        isf_q[i] = dico1_isf_noise[indice[0] * 2 + i]; 
    }
    for (i = 0; i < 3; i++)
    {
        isf_q[i + 2] = dico2_isf_noise[indice[1] * 3 + i];      
    }
    for (i = 0; i < 3; i++)
    {
        isf_q[i + 5] = dico3_isf_noise[indice[2] * 3 + i];      
    }
    for (i = 0; i < 4; i++)
    {
        isf_q[i + 8] = dico4_isf_noise[indice[3] * 4 + i];      
    }
    for (i = 0; i < 4; i++)
    {
        isf_q[i + 12] = dico5_isf_noise[indice[4] * 4 + i];  
    }

    for (i = 0; i < ORDER; i++)
    {
        isf_q[i] = add(isf_q[i], mean_isf_noise[i]); 
    }

    voAMRWBPDecReorder_isf(isf_q, ISF_GAP, ORDER);

    return;
}
