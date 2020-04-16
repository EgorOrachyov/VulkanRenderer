/**********************************************************************************/
/* This file is part of Ignimbrite project                                        */
/* https://github.com/EgorOrachyov/Ignimbrite                                     */
/**********************************************************************************/
/* Licensed under MIT License                                                     */
/* Copyright (c) 2019 - 2020 Egor Orachyov                                        */
/* Copyright (c) 2019 - 2020 Sultim Tsyrendashiev                                 */
/**********************************************************************************/

#include <Material.h>
#include <MeshLoader.h>
#include <NoirFilter.h>
#include <InverseFilter.h>
#include <RenderEngine.h>
#include <RenderableMesh.h>
#include <VulkanExtensions.h>
#include <MaterialFullscreen.h>
#include <VulkanRenderDevice.h>
#include <VertexLayoutFactory.h>

#include <fstream>
#include <stb_image.h>

using namespace ignimbrite;

struct Vertex {
    float32 Position[3];
    float32 Normal[3];
    float32 UV[2];
};

struct Window {
#ifdef __APPLE__
    int32 w = 1280 / 2;
    int32 h = 720 / 2;
#else
    int32 w = 1280;
    int32 h = 720;
#endif
    GLFWwindow* handle;
    ID<IRenderDevice::Surface> surface;
    String name = "Render Engine Test";
    uint32 extensionsCount;
    const char** extensions;
};

class RenderEngineTest {
public:

    RenderEngineTest() {
        init();
    }

    ~RenderEngineTest() {
        shutdown();
    }

    void initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        window.handle = glfwCreateWindow(window.w, window.h, window.name.c_str(), nullptr, nullptr);
        glfwGetFramebufferSize(window.handle, &window.w, &window.h);

        window.extensions = glfwGetRequiredInstanceExtensions(&window.extensionsCount);
    }

    void initDevice() {
        device = std::make_shared<VulkanRenderDevice>(window.extensionsCount, window.extensions);
        window.surface = VulkanExtensions::createSurfaceGLFW((VulkanRenderDevice&)*device, window.handle, window.w, window.h, window.name);
    }

    void initCamera() {
        Mat4f clip = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
                               0.0f, -1.0f, 0.0f, 0.0f,
                               0.0f, 0.0f, 0.5f, 0.0f,
                               0.0f, 0.0f, 0.5f, 1.0f);

        camera = std::make_shared<Camera>();
        camera->setType(Camera::Type::Perspective);
        camera->setAspect((float32)window.w / (float32)window.h);
        camera->setPosition(glm::vec3(0,0, 1));
        camera->rotate(glm::vec3(0,1,0), glm::radians(180.0f));
        camera->setNearView(0.1f);
        camera->setFarView(100.0f);
        camera->setClipMatrix(clip);
        camera->recalculate();
    }

    void initEngine() {
        engine = std::make_shared<RenderEngine>();
        engine->setRenderDevice(device);
        engine->setTargetSurface(window.surface);
        engine->setCamera(camera);
        engine->setRenderArea(0, 0, window.w, window.h);

        auto presentationPass = MaterialFullscreen::fullscreenQuad(PREFIX_PATH, window.surface, device);
        engine->setPresentationPass(presentationPass);
    }

    void initPostEffects() {
        auto inverse = std::make_shared<InverseFilter>(device, PREFIX_PATH);
        engine->addPostEffect(inverse);

        auto noir = std::make_shared<NoirFilter>(device, PREFIX_PATH);
        engine->addPostEffect(noir);
    }

    void initMeshMaterial() {
        // Shader
        std::ifstream vertFile(MODEL3D_SHADER_PATH_VERT.c_str(), std::ios::binary);
        std::ifstream fragFile(MODEL3D_SHADER_PATH_FRAG.c_str(), std::ios::binary);

        std::vector<uint8> vertSpv(std::istreambuf_iterator<char>(vertFile), {});
        std::vector<uint8> fragSpv(std::istreambuf_iterator<char>(fragFile), {});

        RefCounted<Shader> shader = std::make_shared<Shader>(device);
        shader->fromSources(ShaderLanguage::SPIRV, vertSpv, fragSpv);
        shader->reflectData();
        shader->generateUniformLayout();

        // Pipeline
        IRenderDevice::VertexBufferLayoutDesc vertexBufferLayoutDesc = {};
        VertexLayoutFactory::createVertexLayoutDesc(Mesh::VertexFormat::PNT, vertexBufferLayoutDesc);

        RefCounted<GraphicsPipeline> pipeline = std::make_shared<GraphicsPipeline>(device);
        pipeline->setTargetFormat(engine->getOffscreenTargetFormat());
        pipeline->setShader(shader);
        pipeline->setVertexBuffersCount(1);
        pipeline->setVertexBufferDesc(0, vertexBufferLayoutDesc);
        pipeline->setBlendEnable(false);
        pipeline->setDepthTestEnable(true);
        pipeline->setDepthWriteEnable(true);
        pipeline->createPipeline();

        // Sampler
        RefCounted<Sampler> sampler = std::make_shared<Sampler>(device);
        sampler->setHighQualityFiltering();

        // Texture data
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

        if (pixels == nullptr) {
            throw std::runtime_error(std::string("Can't load texture at: ") + MESH_PATH);
        }

        // Texture
        RefCounted<Texture> texture = std::make_shared<Texture>(device);
        texture->setDataAsRGBA8(texWidth, texHeight, pixels, true);
        texture->setSampler(sampler);

        stbi_image_free(pixels);

        // Material
        material = std::make_shared<Material>(device);
        material->setGraphicsPipeline(pipeline);
        material->createMaterial();
        material->setTexture2D("texSampler", texture);
        material->updateUniformData();
    }

    void initMesh() {
        MeshLoader loader(MESH_PATH);
        RefCounted<Mesh> data = loader.importMesh(Mesh::VertexFormat::PNT);

        for (int32 x = -MESH_COUNT_X2; x <= MESH_COUNT_X2; x++) {
            for (int32 z = -MESH_COUNT_Z2; z <= MESH_COUNT_Z2; z++) {
                RefCounted<RenderableMesh> mesh = std::make_shared<RenderableMesh>();
                RefCounted<Material> mat = material->clone();

                mesh->setRenderDevice(device);
                mesh->setRenderMesh(data, false);
                mesh->setRenderMaterial(mat, false);
                mesh->translate(Vec3f(x * MESH_STEP, 0, z * MESH_STEP));
                mesh->create();
                mesh->setVisible(true);
                mesh->setCanApplyCulling(true);
                mesh->setLayerID((uint32) IRenderable::DefaultLayers::Solid);
                mesh->setMaxViewDistance(50.0f);

                engine->addRenderable(mesh);
                meshes.push_back(mesh);

                auto Rx = (float32)rand()/(float32)RAND_MAX - 0.5f;
                auto Ry = (float32)rand()/(float32)RAND_MAX - 0.5f;
                auto Rz = (float32)rand()/(float32)RAND_MAX - 0.5f;
                auto Ra = (float32)rand()/(float32)RAND_MAX - 0.5f;

                rotations.push_back(Vec4f(Rx, Ry, Rz, Ra));
            }
        }
    }

    void inputUpdate() {
        if (glfwGetKey(window.handle, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window.handle, true);

        float32 cameraSpeed = 2.0f / 60.0f;
        float32 cameraRotationSpeed = 0.5f / 60.0f;
        if (glfwGetKey(window.handle, GLFW_KEY_W) == GLFW_PRESS)
            camera->move(cameraSpeed * camera->getDirection());
        if (glfwGetKey(window.handle, GLFW_KEY_S) == GLFW_PRESS)
            camera->move(-cameraSpeed * camera->getDirection());
        if (glfwGetKey(window.handle, GLFW_KEY_A) == GLFW_PRESS)
            camera->move(-camera->getRight() * cameraSpeed);
        if (glfwGetKey(window.handle, GLFW_KEY_D) == GLFW_PRESS)
            camera->move(camera->getRight() * cameraSpeed);
        if (glfwGetKey(window.handle, GLFW_KEY_Q) == GLFW_PRESS)
            camera->move(-camera->getUp() * cameraSpeed);
        if (glfwGetKey(window.handle, GLFW_KEY_E) == GLFW_PRESS)
            camera->move(camera->getUp() * cameraSpeed);
        if (glfwGetKey(window.handle, GLFW_KEY_LEFT) == GLFW_PRESS)
            camera->rotate(glm::vec3(0, 1, 0), cameraRotationSpeed);
        if (glfwGetKey(window.handle, GLFW_KEY_RIGHT) == GLFW_PRESS)
            camera->rotate(glm::vec3(0, 1, 0), -cameraRotationSpeed);

        camera->recalculate();
    }

    void meshUpdate() {
        for (uint32 i = 0; i < meshes.size(); i++) {
            auto& m = meshes[i];
            auto& R = rotations[i];
            m->rotate(glm::vec3(R.x, R.y, R.z), 0.02f * R.w);
            m->updateAABB();
        }
    }

    void init() {
        initWindow();
        initDevice();
        initCamera();
        initEngine();
        initPostEffects();
        initMeshMaterial();
        initMesh();
    }

    void run() {
        while (!glfwWindowShouldClose(window.handle)) {
            glfwPollEvents();
            glfwSwapBuffers(window.handle);
            inputUpdate();
            meshUpdate();
            engine->draw();
        }
    }

    void shutdown() {
        VulkanExtensions::destroySurface((VulkanRenderDevice&)*device, window.surface);
    }

private:
    Window window;
    RefCounted<IRenderEngine>  engine;
    RefCounted<IRenderDevice>  device;
    RefCounted<Camera>         camera;
    RefCounted<Material>       material;

    std::vector<RefCounted<RenderableMesh>> meshes;
    std::vector<Vec4f>                      rotations;

    const int32 MESH_COUNT_X2 = 5;
    const int32 MESH_COUNT_Z2 = 5;
    const int32 MESH_STEP     = 2;

    String PREFIX_PATH = "./shaders/";
    String MODEL3D_SHADER_PATH_VERT = "shaders/spirv/vert3d.spv";
    String MODEL3D_SHADER_PATH_FRAG = "shaders/spirv/frag3d.spv";

    String MESH_PATH = "assets/models/sphere.obj";
    String TEXTURE_PATH = "assets/textures/double.png";

};

int main() {
    RenderEngineTest test;
    test.run();
    return 0;
}