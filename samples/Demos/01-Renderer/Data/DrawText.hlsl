#include "ShaderData.h"

[[vk::push_constant]]
DrawTextShaderParameters parameters : register(b0);

static const uint4 font_data[96] = {
    { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
    { 0x00001010, 0x10101010, 0x00001010, 0x00000000 },
    { 0x00242424, 0x24000000, 0x00000000, 0x00000000 },
    { 0x00000024, 0x247E2424, 0x247E2424, 0x00000000 },
    { 0x00000808, 0x1E20201C, 0x02023C08, 0x08000000 },
    { 0x00000030, 0x494A3408, 0x16294906, 0x00000000 },
    { 0x00003048, 0x48483031, 0x49464639, 0x00000000 },
    { 0x00101010, 0x10000000, 0x00000000, 0x00000000 },
    { 0x00000408, 0x08101010, 0x10101008, 0x08040000 },
    { 0x00002010, 0x10080808, 0x08080810, 0x10200000 },
    { 0x00000000, 0x0024187E, 0x18240000, 0x00000000 },
    { 0x00000000, 0x0808087F, 0x08080800, 0x00000000 },
    { 0x00000000, 0x00000000, 0x00001818, 0x08081000 },
    { 0x00000000, 0x0000007E, 0x00000000, 0x00000000 },
    { 0x00000000, 0x00000000, 0x00001818, 0x00000000 },
    { 0x00000202, 0x04040808, 0x10102020, 0x40400000 },
    { 0x0000003C, 0x42464A52, 0x6242423C, 0x00000000 },
    { 0x00000008, 0x18280808, 0x0808083E, 0x00000000 },
    { 0x0000003C, 0x42020204, 0x0810207E, 0x00000000 },
    { 0x0000007E, 0x04081C02, 0x0202423C, 0x00000000 },
    { 0x00000004, 0x0C142444, 0x7E040404, 0x00000000 },
    { 0x0000007E, 0x40407C02, 0x0202423C, 0x00000000 },
    { 0x0000001C, 0x2040407C, 0x4242423C, 0x00000000 },
    { 0x0000007E, 0x02040408, 0x08101010, 0x00000000 },
    { 0x0000003C, 0x4242423C, 0x4242423C, 0x00000000 },
    { 0x0000003C, 0x4242423E, 0x02020438, 0x00000000 },
    { 0x00000000, 0x00181800, 0x00001818, 0x00000000 },
    { 0x00000000, 0x00181800, 0x00001818, 0x08081000 },
    { 0x00000004, 0x08102040, 0x20100804, 0x00000000 },
    { 0x00000000, 0x00007E00, 0x007E0000, 0x00000000 },
    { 0x00000020, 0x10080402, 0x04081020, 0x00000000 },
    { 0x00003C42, 0x02040810, 0x00001010, 0x00000000 },
    { 0x00001C22, 0x414F5151, 0x51534D40, 0x201F0000 },
    { 0x00000018, 0x24424242, 0x7E424242, 0x00000000 },
    { 0x0000007C, 0x4242427C, 0x4242427C, 0x00000000 },
    { 0x0000001E, 0x20404040, 0x4040201E, 0x00000000 },
    { 0x00000078, 0x44424242, 0x42424478, 0x00000000 },
    { 0x0000007E, 0x4040407C, 0x4040407E, 0x00000000 },
    { 0x0000007E, 0x4040407C, 0x40404040, 0x00000000 },
    { 0x0000001E, 0x20404046, 0x4242221E, 0x00000000 },
    { 0x00000042, 0x4242427E, 0x42424242, 0x00000000 },
    { 0x0000003E, 0x08080808, 0x0808083E, 0x00000000 },
    { 0x00000002, 0x02020202, 0x0242423C, 0x00000000 },
    { 0x00000042, 0x44485060, 0x50484442, 0x00000000 },
    { 0x00000040, 0x40404040, 0x4040407E, 0x00000000 },
    { 0x00000041, 0x63554949, 0x41414141, 0x00000000 },
    { 0x00000042, 0x62524A46, 0x42424242, 0x00000000 },
    { 0x0000003C, 0x42424242, 0x4242423C, 0x00000000 },
    { 0x0000007C, 0x4242427C, 0x40404040, 0x00000000 },
    { 0x0000003C, 0x42424242, 0x4242423C, 0x04020000 },
    { 0x0000007C, 0x4242427C, 0x48444242, 0x00000000 },
    { 0x0000003E, 0x40402018, 0x0402027C, 0x00000000 },
    { 0x0000007F, 0x08080808, 0x08080808, 0x00000000 },
    { 0x00000042, 0x42424242, 0x4242423C, 0x00000000 },
    { 0x00000042, 0x42424242, 0x24241818, 0x00000000 },
    { 0x00000041, 0x41414149, 0x49495563, 0x00000000 },
    { 0x00000041, 0x41221408, 0x14224141, 0x00000000 },
    { 0x00000041, 0x41221408, 0x08080808, 0x00000000 },
    { 0x0000007E, 0x04080810, 0x1020207E, 0x00000000 },
    { 0x00001E10, 0x10101010, 0x10101010, 0x101E0000 },
    { 0x00004040, 0x20201010, 0x08080404, 0x02020000 },
    { 0x00007808, 0x08080808, 0x08080808, 0x08780000 },
    { 0x00001028, 0x44000000, 0x00000000, 0x00000000 },
    { 0x00000000, 0x00000000, 0x00000000, 0x00FF0000 },
    { 0x00201008, 0x04000000, 0x00000000, 0x00000000 },
    { 0x00000000, 0x003C0202, 0x3E42423E, 0x00000000 },
    { 0x00004040, 0x407C4242, 0x4242427C, 0x00000000 },
    { 0x00000000, 0x003C4240, 0x4040423C, 0x00000000 },
    { 0x00000202, 0x023E4242, 0x4242423E, 0x00000000 },
    { 0x00000000, 0x003C4242, 0x7E40403E, 0x00000000 },
    { 0x00000E10, 0x107E1010, 0x10101010, 0x00000000 },
    { 0x00000000, 0x003E4242, 0x4242423E, 0x02023C00 },
    { 0x00004040, 0x407C4242, 0x42424242, 0x00000000 },
    { 0x00000808, 0x00380808, 0x0808083E, 0x00000000 },
    { 0x00000404, 0x001C0404, 0x04040404, 0x04043800 },
    { 0x00004040, 0x40444850, 0x70484442, 0x00000000 },
    { 0x00003808, 0x08080808, 0x0808083E, 0x00000000 },
    { 0x00000000, 0x00774949, 0x49494949, 0x00000000 },
    { 0x00000000, 0x007C4242, 0x42424242, 0x00000000 },
    { 0x00000000, 0x003C4242, 0x4242423C, 0x00000000 },
    { 0x00000000, 0x007C4242, 0x4242427C, 0x40404000 },
    { 0x00000000, 0x003E4242, 0x4242423E, 0x02020200 },
    { 0x00000000, 0x002E3020, 0x20202020, 0x00000000 },
    { 0x00000000, 0x003E4020, 0x1804027C, 0x00000000 },
    { 0x00000010, 0x107E1010, 0x1010100E, 0x00000000 },
    { 0x00000000, 0x00424242, 0x4242423E, 0x00000000 },
    { 0x00000000, 0x00424242, 0x24241818, 0x00000000 },
    { 0x00000000, 0x00414141, 0x49495563, 0x00000000 },
    { 0x00000000, 0x00412214, 0x08142241, 0x00000000 },
    { 0x00000000, 0x00424242, 0x4242423E, 0x02023C00 },
    { 0x00000000, 0x007E0408, 0x1020407E, 0x00000000 },
    { 0x000E1010, 0x101010E0, 0x10101010, 0x100E0000 },
    { 0x00080808, 0x08080808, 0x08080808, 0x08080000 },
    { 0x00700808, 0x08080807, 0x08080808, 0x08700000 },
    { 0x00003149, 0x46000000, 0x00000000, 0x00000000 },
    { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
};

struct Vertex
{
    float4 Position;
    float2 TextureCoordinates;
};

struct VertexOutput
{
    float4 Position: SV_Position;
    float2 TextureCoordinates: TEXCOORD0;
};

static Vertex quadVertices[] =
{
    { float4(-1.0, 1.0, 0.0, 1.0), float2(0.0, 0.0) },
    { float4(1.0, 1.0, 0.0, 1.0), float2(1.0, 0.0) },
    { float4(-1.0, -1.0, 0.0, 1.0), float2(0.0, 1.0) },
    { float4(1.0, -1.0, 0.0, 1.0), float2(1.0, 1.0) }
};

static uint3 quadIndices[] =
{
    uint3(0, 1, 2),
    uint3(2, 1, 3)
};

[shader("mesh")]
[OutputTopology("triangle")]
[NumThreads(32, 1, 1)]
void MeshMain(in uint groupThreadId : SV_GroupThreadID, out vertices VertexOutput vertices[4], out indices uint3 indices[2])
{
    const uint meshVertexCount = 4;
    const uint triangleCount = 2;

    SetMeshOutputCounts(meshVertexCount, triangleCount);

    if (groupThreadId < meshVertexCount)
    {
        vertices[groupThreadId].Position = quadVertices[groupThreadId].Position;
        vertices[groupThreadId].TextureCoordinates = quadVertices[groupThreadId].TextureCoordinates;
    }

    if (groupThreadId < triangleCount)
    {
        indices[groupThreadId] = quadIndices[groupThreadId];
    }
}

float4 DrawTextCharacter(ByteAddressBuffer textBuffer, uint offset, uint2 drawLocalPosition, float3 color)
{
    uint charIndex = drawLocalPosition.x / 8;
    uint lineIndex = drawLocalPosition.y / 16;
    
    uint length = textBuffer.Load<uint>(offset);

    if (charIndex >= length || lineIndex > 0)
    {
        return float4(0.0, 0.0, 0.0, 0.0);
    }

    uint charCodeRaw = textBuffer.Load<uint>(offset + 4 + charIndex);
    uint charCode = unpack_u8u32(charCodeRaw)[charIndex % 4];
    charCode -= 0x20;

    uint2 charCoord = uint2(drawLocalPosition.x % 8, drawLocalPosition.y % 16);

    uint four_lines = font_data[charCode][charCoord.y / 4];

    uint localY = charCoord.y % 4;

    //uint current_line  = (four_lines >> (8*(3-(charCoord.y)%4))) & 0xff;
    uint current_line  = (four_lines >> (8*(3-localY))) & 0xff;
    uint current_pixel = (current_line >> (7-charCoord.x)) & 0x01;
    
    // if the pixel is 0, choose background colour, if it is 1 choose 
    // foreground colour
    return lerp(float4(0, 0, 0, 0), float4(color, 1), current_pixel);
}

[shader("pixel")]
float4 PixelMain(const VertexOutput input) : SV_Target0
{
    ByteAddressBuffer draw2DCommandsBuffer = ResourceDescriptorHeap[parameters.Draw2DCommandsBufferIndex];
    ByteAddressBuffer textBuffer = ResourceDescriptorHeap[parameters.TextBufferIndex];

    float2 pixelPosition = input.TextureCoordinates * parameters.RenderTargetSize;
    float4 finalPixel = float4(0.0, 0.0, 0.0, 0.0);
    
    for (uint i = 0; i < parameters.CommandCount; i++)
    {
        Draw2DCommand command = draw2DCommandsBuffer.Load<Draw2DCommand>(i * sizeof(Draw2DCommand));

        if (pixelPosition.x < command.PositionX || pixelPosition.y < command.PositionY)
        {
            continue;
        }
       
        if (command.Type == Draw2DCommandType_Text)
        {
            uint2 drawLocalPosition = uint2(pixelPosition.x - command.PositionX - 1, pixelPosition.y - command.PositionY - 1);
            finalPixel += DrawTextCharacter(textBuffer, command.CommandDataOffset, drawLocalPosition, float3(0.0, 0.0, 0.0));

            drawLocalPosition = uint2(pixelPosition.x - command.PositionX, pixelPosition.y - command.PositionY);
            finalPixel += DrawTextCharacter(textBuffer, command.CommandDataOffset, drawLocalPosition, float3(1.0, 1.0, 1.0));
        }
    }

    return finalPixel;
}
