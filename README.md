# silk-codec-cpp

Silk codec implemented by C++. Encoding pcm audio to silk, or decoding silk audio to pcm.

# Build

```bash
cmake .
make
./SilkCodecDemo
```

# Example

Please refer to `demo.cpp`

```cpp
#include <stdio.h>
#include "silk_codec.h"

int main()
{
    // demo for silk encoder
    {
        SilkCodec::SilkEncoder encoder;
        encoder.Init(8000, 16, 1);
        int ret = encoder.EncodeFile("8000_16bit_1channel.pcm", "test_out.silk");
        if(ret != 0){
            printf("EncodeFile failed, ret:%d\n", ret);
            return 0;
        }
        printf("encoder success\n");
    }

    // demo for silk decoder
    {
        SilkCodec::SilkDecoder decoder;
        decoder.Init(8000, 16, 1);
        int ret = decoder.DecodeFile("test_out.silk", "test_out.pcm");
        if(ret != 0){
            printf("DecodeFile failed, ret:%d\n", ret);
            return 0;
        }
        printf("decoder success\n");
    }

    return 0;
}
```

# References
> - https://github.com/ploverlake/silk
> - https://github.com/JarvisChu/silk-codec

