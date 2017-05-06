
#include "utility.h"
#include "RTPPacket.h"
#include "H264VideoRTPParser.h"
#include "MediaStream.h"

#ifdef _VONAMESPACE
using namespace _VONAMESPACE;
#endif

static const int MAX_FRAME_SIZE = 1024 * 256;

CH264VideoRTPParser::CH264VideoRTPParser(CMediaStream * mediaStream, CMediaStreamSocket * rtpStreamSock)
: CRTPParser(mediaStream, rtpStreamSock)
{
	m_packetNALUnitType = 0;
}

CH264VideoRTPParser::~CH264VideoRTPParser()
{

}

bool CH264VideoRTPParser::ParseRTPPayloadHeader(CRTPPacket * rtpPacket)
{
	int frameHeaderSize = 0;
	unsigned char *pos=rtpPacket->RTPData();
	if (!m_mediaStream->IsVideoH264AVC())
	{
		m_packetNALUnitType = pos[0] & 0x1F;
	}
	else
		m_packetNALUnitType = 0;
	
	switch(m_packetNALUnitType) 
	{
	case 24: 
		{ // STAP-A
			frameHeaderSize = 1;
			break;
		}
	case 25:
	case 26:
	case 27: 
		{ // STAP-B, MTAP16, or MTAP24
			frameHeaderSize = 3;
			break;
		}
	case 28: 
	case 29:
		{ // FU-A or FU-B
			m_firstPacketInMultiPacketFrame = (rtpPacket->RTPData()[1] & 0x80) != 0;
			m_lastPacketInMultiPacketFrame = (rtpPacket->RTPData()[1] & 0x40) != 0;

			if(m_firstPacketInMultiPacketFrame) 
			{
				frameHeaderSize = 1;
				if(rtpPacket->RTPDataSize() < frameHeaderSize)
					return false;

				rtpPacket->RTPData()[1] = (rtpPacket->RTPData()[0] & 0xE0) + (rtpPacket->RTPData()[1] & 0x1F); 
			} 
			else
			{
				frameHeaderSize = 2;
				if(rtpPacket->RTPDataSize() < frameHeaderSize)
					return false;

				m_firstPacketInMultiPacketFrame = false;
			}
			break;
		}
	default:
		{
			m_firstPacketInMultiPacketFrame = m_lastPacketInMultiPacketFrame = true;
			break;
		}
	}

	rtpPacket->Skip(frameHeaderSize);

	return true;
}

bool CH264VideoRTPParser::ParseRTPPayloadFrame(CRTPPacket * rtpPacket) 
{
	if(rtpPacket->RTPDataSize() == 0)
		return false;

	int resultNALUSize = 0;
	switch(m_packetNALUnitType) 
	{
	case 24:
	case 25:
		{ // STAP-A or STAP-B
			if(rtpPacket->RTPDataSize() < 2)
				break;

			resultNALUSize = (rtpPacket->RTPData()[0] << 8) | rtpPacket->RTPData()[1];
			rtpPacket->Skip(2);
			break;
		}
	case 26: 
		{ // MTAP16
			if(rtpPacket->RTPDataSize() < 5)
				break;

			resultNALUSize = (rtpPacket->RTPData()[0] << 8) | rtpPacket->RTPData()[1];
			rtpPacket->Skip(5);
			break;
		}
	case 27: 
		{ // MTAP24
			if(rtpPacket->RTPDataSize() < 6) 
				break;

			resultNALUSize = (rtpPacket->RTPData()[0] << 8) | rtpPacket->RTPData()[1];
			rtpPacket->Skip(6);
			break;
		}
	default: 
		{
			resultNALUSize = rtpPacket->RTPDataSize();
		} 
	}

	int payloadFrameSize = (resultNALUSize <= rtpPacket->RTPDataSize()) ? resultNALUSize : rtpPacket->RTPDataSize();

	if((payloadFrameSize+m_frameSize)>MAX_FRAME_SIZE)
	{
		rtpPacket->Skip(payloadFrameSize);

		sprintf(CLog::formatString,"frameSize(%d)>MAX_FRAME_SIZE\n",payloadFrameSize+m_frameSize);
		CLog::Log.MakeLog(LL_RTSP_ERR,"error.txt",CLog::formatString);
		return false;
	}

	memcpy(m_frameData + m_frameSize, rtpPacket->RTPData(), payloadFrameSize);
	m_frameSize += payloadFrameSize;
	rtpPacket->Skip(payloadFrameSize);

	return payloadFrameSize > 0;
}
