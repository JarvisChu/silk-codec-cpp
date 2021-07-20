#include <stdio.h>
#include "silk_codec.h"
#include "silk/interface/SKP_Silk_SDK_API.h"

#define MAX_INPUT_FRAMES        5
#define FRAME_LENGTH_MS         20
#define MAX_API_FS_KHZ          48

namespace SilkCodec {

/* SwapEndian: convert a little endian int16 to a big endian int16 or vica verca
 * vec: int16 array
 * len: length of vec
 */
void SwapEndian(int16_t vec[],int len)
{
    for(int i = 0; i < len; i++){
        int16_t tmp = vec[i];
        uint8_t *p1 = (uint8_t *) &vec[i]; 
        uint8_t *p2 = (uint8_t *) &tmp;
        p1[0] = p2[1]; 
        p1[1] = p2[0];
    }
}

/* IsBigEndian: check system endian
 */
bool IsBigEndian()
{
    uint16_t n = 1;
    if( ((uint8_t*) &n)[0] == 1 ){
        return false;
    }
    return true;
}

void Short2LittleEndianBytes(short st, uint8_t bs[2])
{
    // change to little endian
    if( IsBigEndian()){
        SwapEndian(&st, 1);
    }

    bs[0] = ((uint8_t*)&st)[0];
    bs[1] = ((uint8_t*)&st)[1];
}

short LittleEndianBytes2Short(uint8_t low, uint8_t high)
{
    short st = 0;
    ((uint8_t*)&st)[0] = low;
    ((uint8_t*)&st)[1] = high;

    if(IsBigEndian()){
        SwapEndian(&st,1);
    }

    return st;
}

SilkEncoder::SilkEncoder()
{
}

SilkEncoder::~SilkEncoder()
{
    if (m_pEncoder) {
		free(m_pEncoder);
		m_pEncoder = nullptr;
	}
}

int SilkEncoder::Init(uint32_t sampleRate, uint32_t sampleBits, uint32_t channelCnt)
{
    printf("[SilkEncoder::Init] sampleRate:%d, sampleBits:%d, channelCnt=%d\n", sampleRate, sampleBits, channelCnt);

	//check sampleRate
	if(sampleRate != 8000 && sampleRate != 16000 && sampleRate != 24000 && 
		sampleRate != 32000 && sampleRate != 44100 && sampleRate != 48000){
		printf("invalid sample rate, only support 8000/16000/24000/32000/44100/48000 \n");
		return ErrCodeInvalidParam;
	}

    // check sampleBits
	if(sampleBits != 8 && sampleBits != 16 && sampleBits != 24 && sampleBits != 32){
		printf("invalid sample bit, only support 8/16/24/32\n");
		return ErrCodeInvalidParam;
	}

	// check channelCnt
	if(channelCnt != 1 && channelCnt != 2 && channelCnt != 4 && channelCnt != 8){
		printf("invalid channel number, only support 1/2/4/8\n");
		return ErrCodeInvalidParam;
	}

    // get encoder size
    SKP_int32 encSizeBytes = 0;
	int ret = SKP_Silk_SDK_Get_Encoder_Size(&encSizeBytes);
	if (ret != 0) {
        printf("SKP_Silk_SDK_Get_Encoder_Size failed, ret:%d\n", ret);
		return ErrCodeGetEncoderSizeFailed;
	}

    // init encoder
    m_pEncoder = malloc(encSizeBytes);
    SKP_SILK_SDK_EncControlStruct encStatus;
    ret = SKP_Silk_SDK_InitEncoder(m_pEncoder, &encStatus);
    if (ret != 0) {
        printf("SKP_Silk_SDK_InitEncoder failed, ret: %d\n", ret); 
        return ErrCodeInitEncoderFailed;
    }

    // save sample parameters
    m_sampleRate = sampleRate;
    m_sampleBits = sampleBits;
    m_channelCnt = channelCnt;

    return ErrCodeSuccess;
}

int SilkEncoder::Encode(const std::vector<uint8_t>& pcmIn, std::vector<uint8_t>& silkOut2Append, uint32_t bitRate/* = 10000*/)
{
    if(m_pEncoder == nullptr) return ErrCodeUninitialized;

    SKP_SILK_SDK_EncControlStruct encControl;
	encControl.API_sampleRate = m_sampleRate;
	encControl.maxInternalSampleRate = m_sampleRate;
	encControl.packetSize = (20 * m_sampleRate) / 1000; // 20ms
	encControl.complexity = 2;
	encControl.packetLossPercentage = 0;
	encControl.useInBandFEC = 0;
	encControl.useDTX = 0;
	encControl.bitRate = bitRate;

    // encode pcm to silk, in standard 20ms format
	size_t nBytesPer20ms = m_sampleRate * m_channelCnt * m_sampleBits / 8 / 50;
    size_t pcmLen = pcmIn.size();
    size_t index = 0;
    short nBytes = 2048;
	SKP_uint8 payload[2048];

    while(index + nBytesPer20ms <= pcmLen){
        // encode 20ms audio data each package

        short nBytes = 2048;
        memset(payload, 0, 2048);
        int ret = SKP_Silk_SDK_Encode(m_pEncoder, &encControl, (const short*)&(pcmIn[index]), encControl.packetSize, payload, &nBytes);
	    if (ret) {
            printf("SKP_Silk_Encode failed, ret: %d\n", ret);
		    return ErrCodeEncodeFailed;
	    }

        uint8_t bytes[2];
	    Short2LittleEndianBytes(nBytes, bytes);
	    silkOut2Append.insert(silkOut2Append.end(), &bytes[0], &bytes[2]); // silk data len
	    silkOut2Append.insert(silkOut2Append.end(), &payload[0], &payload[nBytes]); // silk data

        index += nBytesPer20ms;
    }

    return ErrCodeSuccess;
}

int SilkEncoder::EncodeFile(const std::string& pcmFilePath, const std::string& silkFilePath, uint32_t bitRate/* = 10000*/)
{
    printf("[SilkEncoder::EncodeFile] pcmFilePath:%s, silkFilePath:%s, bitRate:%d\n", pcmFilePath.c_str(), silkFilePath.c_str(), bitRate);
    
    if(m_pEncoder == nullptr) return ErrCodeUninitialized;

    // open pcm file
    FILE* fpIn = fopen(pcmFilePath.c_str(), "rb");
	if (!fpIn) {
        printf("open pcm file %s failed\n", pcmFilePath.c_str());
        return ErrCodeOpenFileFailed;
	}

    // open silk file
    FILE* fpOut = fopen(silkFilePath.c_str(), "wb");
	if (!fpOut) {
        printf("open silk file %s failed\n", silkFilePath.c_str());
        fclose(fpIn);
        return ErrCodeOpenFileFailed;
	}

    // write silk file header
    const char* header = "#!SILK_V3";
    fwrite(header, sizeof(char), strlen(header), fpOut);

    // read from pcm file, encode, save to silk file
    size_t nBytesPer20ms = m_sampleRate * m_channelCnt * m_sampleBits / 8 / 50;
    size_t nBufferSize = 1000 * nBytesPer20ms; // read 1000*20ms = 20s
    uint8_t* buffer = (uint8_t*) malloc(sizeof(uint8_t) * nBufferSize);

    int retCode = ErrCodeSuccess;
    while(true){
		int nReadSize = fread(buffer, sizeof(uint8_t), nBufferSize, fpIn);
		if (nReadSize <= 0) break;

        std::vector<uint8_t> pcmIn(buffer, buffer + nReadSize);
        std::vector<uint8_t> silkOut;

        int ret = Encode(pcmIn, silkOut, bitRate);
        if( ret != ErrCodeSuccess ){
            printf("encode failed, ret:%d\n", ret);
            retCode = ErrCodeEncodeFailed;
            break;
        }

        fwrite(&silkOut[0], sizeof(uint8_t), silkOut.size(), fpOut);
    }

    if(buffer) {
        free(buffer);
        buffer = nullptr;
    }

    fclose(fpIn);
    fclose(fpOut);

    return retCode;
}

////////////////////////////////
// SilkDecoder

SilkDecoder::SilkDecoder()
{
}

SilkDecoder::~SilkDecoder()
{
    if (m_pDecoder) {
		free(m_pDecoder);
		m_pDecoder = nullptr;
	}
}

int SilkDecoder::Init(uint32_t sampleRate, uint32_t sampleBits, uint32_t channelCnt)
{
    printf("[SilkDecoder::Init] sampleRate:%d, sampleBits:%d, channelCnt=%d\n", sampleRate, sampleBits, channelCnt);

	//check sampleRate
	if(sampleRate != 8000 && sampleRate != 16000 && sampleRate != 24000 && 
		sampleRate != 32000 && sampleRate != 44100 && sampleRate != 48000){
		printf("invalid sample rate, only support 8000/16000/24000/32000/44100/48000 \n");
		return ErrCodeInvalidParam;
	}

    // check sampleBits
	if(sampleBits != 8 && sampleBits != 16 && sampleBits != 24 && sampleBits != 32){
		printf("invalid sample bit, only support 8/16/24/32\n");
		return ErrCodeInvalidParam;
	}

	// check channelCnt
	if(channelCnt != 1 && channelCnt != 2 && channelCnt != 4 && channelCnt != 8){
		printf("invalid channel number, only support 1/2/4/8\n");
		return ErrCodeInvalidParam;
	}

    // get decoder size
    int decSizeBytes = 0;
    int ret = SKP_Silk_SDK_Get_Decoder_Size(&decSizeBytes);
	if (ret != 0) {
        printf("SKP_Silk_SDK_Get_Decoder_Size failed, ret:%d\n", ret);
		return ErrCodeGetDecoderSizeFailed;
	}

    // init decoder
    m_pDecoder = malloc(decSizeBytes);
    ret = SKP_Silk_SDK_InitDecoder(m_pDecoder);
    if (ret != 0) {
        printf("SKP_Silk_SDK_InitDecoder failed, ret: %d\n", ret); 
        return ErrCodeInitDecoderFailed;
    }

    // save sample parameters
    m_sampleRate = sampleRate;
    m_sampleBits = sampleBits;
    m_channelCnt = channelCnt;

    return ErrCodeSuccess;
}

int SilkDecoder::Decode(const std::vector<uint8_t>& silkIn, std::vector<uint8_t>& pcmOut2Append)
{
    if(m_pDecoder == nullptr) return ErrCodeUninitialized;

    size_t index = 0;
    while(index + 2 < silkIn.size()){
        // decode one package
        short pkgSize = LittleEndianBytes2Short(silkIn[index], silkIn[index+1]);
        std::vector<uint8_t> oneSilkPackage(silkIn.begin() + index + 2, silkIn.begin() + index + 2 + pkgSize);
        int ret = DecodeRaw(oneSilkPackage, pcmOut2Append);
        if(ret != ErrCodeSuccess){
            printf("DecodeRaw failed, ret:%d\n", ret);
            return ErrCodeDecodeFailed;
        }

        index += 2 + pkgSize;
    }

    return ErrCodeSuccess;
}

int SilkDecoder::DecodeRaw(const std::vector<uint8_t>& silkIn, std::vector<uint8_t>& pcmOut2Append)
{
    SKP_SILK_SDK_DecControlStruct decControl;
    decControl.API_sampleRate = (int)m_sampleRate; // I: Output signal sampling rate in Hertz; 8000/12000/16000/24000
    
    int frames = 0;
    SKP_int16 out[ ( ( FRAME_LENGTH_MS * MAX_API_FS_KHZ ) << 1 ) * MAX_INPUT_FRAMES ] = {0};
    SKP_int16 *outPtr = out;
    short out_size = 0;
    short len = 0;
    do {
        /* Decode 20 ms */
        int ret = SKP_Silk_SDK_Decode(m_pDecoder, &decControl, 0, (unsigned char*) silkIn.data(), (int)silkIn.size(), outPtr, &len);
        if( ret ) {
            printf( "SKP_Silk_SDK_Decode failed, ret: %d\n", ret);
            return ErrCodeDecodeFailed;
        }

        frames++;
        outPtr += len;
        out_size += len;
        if( frames > MAX_INPUT_FRAMES ) {
            /* Hack for corrupt stream that could generate too many frames */
            outPtr     = out;
            out_size = 0;
            frames = 0;
            break;
        }
    /* Until last 20 ms frame of packet has been decoded */
    } while( decControl.moreInternalDecoderFrames);

    // using little endian
    if( IsBigEndian()){
        SwapEndian(out, out_size);
    }

    // append to pcm out
    uint8_t* p = (uint8_t*)out;
    pcmOut2Append.insert(pcmOut2Append.end(), p, p + out_size*2 );

    return ErrCodeSuccess;
}

int SilkDecoder::DecodeFile(const std::string& silkFilePath, const std::string& pcmFilePath)
{
    printf("[SilkDecoder::DecodeFile] silkFilePath:%s, pcmFilePath:%s\n", silkFilePath.c_str(), pcmFilePath.c_str());
    
    if(m_pDecoder == nullptr) return ErrCodeUninitialized;

    // open silk file
    FILE* fpIn = fopen(silkFilePath.c_str(), "rb");
	if (!fpIn) {
        printf("open silk file %s failed\n", silkFilePath.c_str());
        return ErrCodeOpenFileFailed;
	}

    // check silk file header
    char header[50];
    int nReadSize = fread(header, sizeof(char), strlen( "#!SILK_V3" ), fpIn);
    header[ strlen( "#!SILK_V3" ) ] = '\0';
    if( strcmp( header, "#!SILK_V3" ) != 0 ) { 
        printf("Invalid Silk Header %s\n", header);
        fclose(fpIn);
        return ErrCodeInvalidSilkFile;
    }

    // open pcm file
    FILE* fpOut = fopen(pcmFilePath.c_str(), "wb");
	if (!fpOut) {
        printf("open pcm file %s failed\n", pcmFilePath.c_str());
        fclose(fpIn);
        return ErrCodeOpenFileFailed;
	}

    // read from silk file, decode, save to pcm file
    int retCode = ErrCodeSuccess;
    while(true){
        // read on package silk data
        char pkgSizeBuffer[2] = {0};
        nReadSize = fread(pkgSizeBuffer, sizeof(char), 2, fpIn);
        if(nReadSize < 2) break;
        short pkgSize = LittleEndianBytes2Short(pkgSizeBuffer[0], pkgSizeBuffer[1]);

        std::vector<uint8_t> oneSilkPackage;
        oneSilkPackage.resize(pkgSize, 0);
        nReadSize = fread( &oneSilkPackage[0] , sizeof(uint8_t), pkgSize, fpIn);
        if(nReadSize < pkgSize){
            break;
        }

        std::vector<uint8_t> pcmOut2Append;
        int ret = DecodeRaw(oneSilkPackage, pcmOut2Append);
        if(ret != ErrCodeSuccess){
            printf("DecodeRaw failed, ret:%d\n", ret);
            retCode = ret;
            break;
        }

        fwrite(&pcmOut2Append[0], sizeof(uint8_t), pcmOut2Append.size(), fpOut);
    }

    fclose(fpIn);
    fclose(fpOut);

    return retCode;
}

} // namespace SilkCodec