// aac: adts.txt(或者enc 设置为aacpara.adtsUsed = 1)
//mp3: dump
//amr:就是一个头("#!AMR\n")

#include "CAACWriter.h"

CAACWriter::CAACWriter(VO_MEM_OPERATOR* pMemOp, VO_FILE_OPERATOR* pFileOp)
  : CvoBaseMemOpr(pMemOp)
  , CvoBaseFileOpr(pFileOp)
  , m_nDataSize(0)
  , m_hFile(NULL)
  , mnTotalFrames(0)
  , mAudioCodec(VO_AUDIO_CodingAAC)
{
	mAudioFormat.SampleRate = 8000;
	mAudioFormat.Channels   = 1;
	mAudioFormat.SampleBits = 16;
}

CAACWriter::~CAACWriter()
{
	Close();
}

VO_U32 CAACWriter::Open(VO_FILE_SOURCE *pSource, VO_SINK_OPENPARAM * pParam)
{
	Close();
	// from mp4
	mAudioCodec    = pParam->nAudioCoding; // TODO: switch to AudioCodec if needed
	
	m_nDataSize    = 0;
	mnTotalFrames  = 0;

	m_hFile = FileOpen(pSource);
	if (!m_hFile)
		return VO_ERR_SINK_OPENFAIL;
	
	return VO_ERR_NONE;
}

VO_U32 CAACWriter::Close()
{
	if(m_hFile == NULL)
		return VO_ERR_NONE;
	
	FileClose(m_hFile);
	m_hFile = NULL;
	return VO_ERR_NONE;
}

VO_U32 CAACWriter::SetParam(VO_U32 uID, VO_PTR pParam)
{
	if(uID == VO_PID_AUDIO_FORMAT) {
		MemCopy(&mAudioFormat, pParam, sizeof(mAudioFormat));
	}
	
	return VO_ERR_NONE;
}

VO_U32 CAACWriter::GetParam(VO_U32 uID, VO_PTR pParam)
{
	if(uID == VO_PID_AUDIO_FORMAT) {
		MemCopy(pParam, &mAudioFormat, sizeof(mAudioFormat));
	}

	return VO_ERR_NONE;
}

VO_U32 CAACWriter::AddSample(VO_SINK_SAMPLE * pSample)
{
	if(pSample == NULL) return VO_ERR_FAILED;

	pSample->Size = pSample->Size & 0x7FFFFFFF;
	
	FileWrite(m_hFile, pSample->Buffer, pSample->Size);
	m_nDataSize += pSample->Size;
	mnTotalFrames++;
	
	return VO_ERR_NONE;
}




