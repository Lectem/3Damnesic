/**
 *@file gpu.c
 *@author Lectem
 *@date 11/07/2015
 */

#include <3ds.h>
#include <gpu_math.h>
#include <shader_shbin.h>
#include <assert.h>
#include <movie.h>

//TODO clean this crap



typedef struct
{
    float x, y;
} vector_2f;

typedef struct
{
    float x, y, z;
} vector_3f;

typedef struct
{
    float r, g, b, a;
} vector_4f;

typedef struct
{
    u8 r, g, b, a;
} vector_4u8;

typedef struct __attribute__((__packed__))
{
    vector_3f position;
    vector_4u8 color;
    vector_2f texpos;
} vertex_pos_col;

#define BG_COLOR_U8 {0xFF,0xFF,0xFF,0xFF}

static const vertex_pos_col test_mesh[] =
        {
                {{-1.0f, -1.0f, -0.5f}, BG_COLOR_U8, {1.0f, 0.0f}},
                {{1.0f,  -1.0f, -0.5f}, BG_COLOR_U8, {1.0f, 1.0f}},
                {{1.0f,  1.0f,  -0.5f}, BG_COLOR_U8, {0.0f, 1.0f}},
                {{-1.0f, 1.0f,  -0.5f}, BG_COLOR_U8, {0.0f, 0.0f}},
        };

static void *test_data = NULL;


void setTexturePart(vertex_pos_col *data, float x, float y, float u, float v)
{
    data[0].texpos.x = u;
    data[0].texpos.y = y;

    data[1].texpos.x = u;
    data[1].texpos.y = v;

    data[2].texpos.x = x;
    data[2].texpos.y = v;

    data[3].texpos.x = x;
    data[3].texpos.y = y;


}


#define ABGR8(r, g, b, a) ((((r)&0xFF)<<24) | (((g)&0xFF)<<16) | (((b)&0xFF)<<8) | (((a)&0xFF)<<0))

u32 clearColor = ABGR8(0x68, 0xB0, 0xD8, 0xFF);

#define DISPLAY_TRANSFER_FLAGS \
    (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
     GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(gfxGetScreenFormat(GFX_TOP)) | \
     GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))


#define GPU_CMD_SIZE 0x40000

//GPU framebuffer address
u32 *gpuFBuffer = NULL;
//GPU depth buffer address
u32 *gpuDBuffer = NULL;
//GPU command buffers
u32 *gpuCmd = NULL;

//shader structure
DVLB_s *shader_dvlb;    //the header
shaderProgram_s shader; //the program


Result projUniformRegister = -1;
Result modelviewUniformRegister = -1;

//The projection matrix
static float ortho_matrix[4 * 4];

void GPU_SetDummyTexEnv(u8 num);

void gpuEndFrame();


void GPU_SetDummyTexEnv(u8 num)
{
    //Don't touch the colors of the previous stages
    GPU_SetTexEnv(num,
                  GPU_TEVSOURCES(GPU_PREVIOUS, 0, 0),
                  GPU_TEVSOURCES(GPU_PREVIOUS, 0, 0),
                  GPU_TEVOPERANDS(0, 0, 0),
                  GPU_TEVOPERANDS(0, 0, 0),
                  GPU_REPLACE,
                  GPU_REPLACE,
                  0xFFFFFFFF);
}


void gpuDisableEverything()
{

    GPU_SetFaceCulling(GPU_CULL_NONE);
    //No stencil test
    GPU_SetStencilTest(false, GPU_ALWAYS, 0x00, 0xFF, 0x00);
    GPU_SetStencilOp(GPU_STENCIL_KEEP, GPU_STENCIL_KEEP, GPU_STENCIL_KEEP);
    //No blending color
    GPU_SetBlendingColor(0, 0, 0, 0);
    //Fake disable AB. We just ignore the Blending part
    GPU_SetAlphaBlending(
            GPU_BLEND_ADD,
            GPU_BLEND_ADD,
            GPU_ONE, GPU_ZERO,
            GPU_ONE, GPU_ZERO
    );

    GPU_SetAlphaTest(false, GPU_ALWAYS, 0x00);

    GPU_SetDepthTestAndWriteMask(true, GPU_ALWAYS, GPU_WRITE_ALL);
    GPUCMD_AddMaskedWrite(GPUREG_0062, 0x1, 0);
    GPUCMD_AddWrite(GPUREG_0118, 0);

    GPU_SetDummyTexEnv(0);
    GPU_SetDummyTexEnv(1);
    GPU_SetDummyTexEnv(2);
    GPU_SetDummyTexEnv(3);
    GPU_SetDummyTexEnv(4);
    GPU_SetDummyTexEnv(5);
}

void gpuInit()
{
    test_data = linearAlloc(sizeof(test_mesh));     //allocate our vbo on the linear heap
    memcpy(test_data, test_mesh, sizeof(test_mesh)); //Copy our data
    //Allocate the GPU render buffers
    gpuFBuffer = vramMemAlign(400 * 240 * 4 * 2 * 2, 0x100);
    gpuDBuffer = vramMemAlign(400 * 240 * 4 * 2 * 2, 0x100);


    //In this example we are only rendering in "2D mode", so we don't need one command buffer per eye
    gpuCmd = linearAlloc(GPU_CMD_SIZE * (sizeof *gpuCmd)); //Don't forget that commands size is 4 (hence the sizeof)

    GPU_Init(NULL);//initialize GPU

    gfxSet3D(false);//We will not be using the 3D mode in this example
    Result res = 0;


    //Reset the gpu
    //This actually needs a command buffer to work, and will then use it as default
    GPU_Reset(NULL, gpuCmd, GPU_CMD_SIZE);

    /**
    * Load our vertex shader and its uniforms
    * Check http://3dbrew.org/wiki/SHBIN for more informations about the shader binaries
    */
    shader_dvlb = DVLB_ParseFile((u32 *) shader_shbin, shader_shbin_size);//load our vertex shader binary
    shaderProgramInit(&shader);
    res = shaderProgramSetVsh(&shader, &shader_dvlb->DVLE[0]);

    projUniformRegister = shaderInstanceGetUniformLocation(shader.vertexShader, "projection");


    shaderProgramUse(&shader); // Select the shader to use



    initOrthographicMatrix(ortho_matrix, 0.0f, 240.0f, 0.0f, 400.0f, 0.0f, 1.0f); // A basic projection for 2D drawings
    SetUniformMatrix(projUniformRegister, ortho_matrix); // Upload the matrix to the GPU


    gpuDisableEverything();
    gpuEndFrame();
}

void gpuExit()
{

    if (test_data)
    {
        linearFree(test_data);
    }
    //do things properly
    linearFree(gpuCmd);
    vramFree(gpuDBuffer);
    vramFree(gpuFBuffer);
    shaderProgramFree(&shader);
    DVLB_Free(shader_dvlb);
    GPU_Reset(NULL, gpuCmd, GPU_CMD_SIZE); // Not really needed, but safer for the next applications ?
}

void gpuEndFrame()
{
    //Ask the GPU to draw everything (execute the commands)
    GPU_FinishDrawing();
    GPUCMD_Finalize();
    GPUCMD_FlushAndRun(NULL);
    gspWaitForP3D();//Wait for the gpu 3d processing to be done
    //Copy the GPU output buffer to the screen framebuffer
    //See http://3dbrew.org/wiki/GPU#Transfer_Engine for more details about the transfer engine
    Result res = GX_SetDisplayTransfer(NULL, // Use ctrulib's gx command buffer
                                       gpuFBuffer, GX_BUFFER_DIM(240, 400),
                                       (u32 *) gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL),
                                       GX_BUFFER_DIM(240, 400),
                                       DISPLAY_TRANSFER_FLAGS);
    if (!res)gspWaitForPPF();

    gfxSwapBuffersGpu();

    //Wait for the screen to be updated
    gspWaitForEvent(GSPEVENT_VBlank0, false);
    //TODO : use gspWaitForEvent(GSPEVENT_VBlank0,true); if tearing happens

    //Clear the screen
    GX_SetMemoryFill(NULL,
                     gpuFBuffer, clearColor, &gpuFBuffer[400 * 240], GX_FILL_TRIGGER | GX_FILL_32BIT_DEPTH,
                     gpuDBuffer, 0x00000000, &gpuDBuffer[400 * 240], GX_FILL_TRIGGER | GX_FILL_32BIT_DEPTH);
    gspWaitForPSC0();

    //Get ready to start a new frame
    GPUCMD_SetBufferOffset(0);
    //Viewport (http://3dbrew.org/wiki/GPU_Commands#Command_0x0041)
    GPU_SetViewport((u32 *) osConvertVirtToPhys((u32) gpuDBuffer),
                    (u32 *) osConvertVirtToPhys((u32) gpuFBuffer),
                    0, 0,
            //Our screen is 400*240, but the GPU actually renders to 400*480 and then downscales it SetDisplayTransfer bit 24 is set
            //This is the case here (See http://3dbrew.org/wiki/GPU#0x1EF00C10 for more details)
                    240, 400);

}


void gpuRenderFrame(MovieState *mvS)
{
    GPU_SetTextureEnable(GPU_TEXUNIT0);

    GPU_SetTexture(
            GPU_TEXUNIT0,
            (u32 *) osConvertVirtToPhys((u32) mvS->outFrame->data[0]),
            // width and height swapped?
            mvS->outFrame->width,
            mvS->outFrame->height,
            //GPU_TEXTURE_MAG_FILTER(GPU_NEAREST) | GPU_TEXTURE_MIN_FILTER(GPU_NEAREST) |
            GPU_TEXTURE_WRAP_S(1) | GPU_TEXTURE_WRAP_T(1),
            GPU_RGBA8
    );
    GPUCMD_AddWrite(GPUREG_TEXUNIT0_BORDER_COLOR, 0xFFFF0000);

    int texenvnum = 0;
    GPU_SetTexEnv(
            texenvnum,
            GPU_TEVSOURCES(GPU_TEXTURE0, GPU_TEXTURE0, 0),
            GPU_TEVSOURCES(GPU_TEXTURE0, GPU_TEXTURE0, 0),
            GPU_TEVOPERANDS(0, 0, 0),
            GPU_TEVOPERANDS(0, 0, 0),
            GPU_REPLACE, GPU_REPLACE,
            0xAABBCCDD
    );

    setTexturePart(test_data, 0.0, 1.0f - mvS->pCodecCtx->height / (float) mvS->outFrame->height,
                   mvS->pCodecCtx->width / (float) mvS->outFrame->width, 1.0f);
//    setTexturePart(test_data,0.0,0.0,1.0,1.0);
    GPU_SetAttributeBuffers(
            3, // number of attributes
            (u32 *) osConvertVirtToPhys((u32) test_data),
            GPU_ATTRIBFMT(0, 3, GPU_FLOAT) | GPU_ATTRIBFMT(1, 4, GPU_UNSIGNED_BYTE) | GPU_ATTRIBFMT(2, 2, GPU_FLOAT),
            0xFFF8,//Attribute mask, in our case 0b1110 since we use only the first one
            0x210,//Attribute permutations (here it is the identity)
            1, //number of buffers
            (u32[]) {0x0}, // buffer offsets (placeholders)
            (u64[]) {0x210}, // attribute permutations for each buffer (identity again)
            (u8[]) {3} // number of attributes for each buffer
    );

    //Display the buffers data
    GPU_DrawArray(GPU_TRIANGLE_FAN, sizeof(test_mesh) / sizeof(test_mesh[0]));

}