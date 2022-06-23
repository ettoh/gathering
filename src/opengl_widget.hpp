#include <glad/glad.h>
#include <stdio.h>
#include <stdlib.h>

#include <map>
#include <string>
#include <vector>

#include "opengl_primitives.hpp"
#include "scene.hpp"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "meta.hpp"

namespace gathering {

constexpr auto PI2 = 2 * M_PI;
constexpr GLuint MAX_ELEMENT_ID = 4294967295;
constexpr glm::vec3 CAMERA_START = glm::vec3(0);

class OpenGLWidget {
   public:
    OpenGLWidget();
    ~OpenGLWidget();
    OpenGLWidget(const OpenGLWidget&) = delete;
    OpenGLWidget& operator=(const OpenGLWidget&) = delete;
    void prepareInstance(SceneData& scene);
    bool closed() const { return !window_visible; }

    /**
     * @brief
     * Note: If the glwf window is not visible, this function won't do anything. You can use
     * #define GATHERING_AUTO_HEADLESS to avoid this behavior.
     */
    void renderFrame();

    /**
     * @brief
     * Note: If the glwf window is not visible, this function won't do anything. You can use
     * #define GATHERING_AUTO_HEADLESS to avoid this behavior.
     */
    void updateScene(const SceneData& scene);
    void setWindowVisibility(const bool is_visible);
    void setImageMode(bool is_imagemode);
    void setView(const glm::mat4& view) { this->view = view; }
    void setProjection(const glm::mat4& projection) { this->projection = projection; }
    bool isPrepared() const { return is_prepared; };

   private:
    void init();
    void initEventHandler();
    void destroy();
    void buildGUI();
    void renderScene();
    void deleteInstance();
    void pushStaticSceneToGPU(const std::vector<OpenGLPrimitives::Object>& scene_objects);
    OpenGLPrimitives::ObjectInfo* getObjectInfo(const std::string& name);

    static void glfw_error_callback(int error, const char* description) {
        fprintf(stderr, "Glfw Error %d: %s\n", error, description);
    }

   private:
    GLFWwindow* window = nullptr;
    StopWatch<std::chrono::microseconds> stop_watch = StopWatch<std::chrono::microseconds>();
    bool window_visible = true;  // whether the glfw window is visible or not
    glm::vec3 clear_color = glm::vec3(0.09f);
    bool is_image_mode = false;

    // handler
    GLuint mvp_prog = 0u, mvp_prog_non_shaded = 0u, static_prog = 0u;
    GLuint vbo_static = 0u, ibo_static = 0u, vbo_uniforms = 0u;
    GLuint vao = 0u;
    OpenGLPrimitives::GLBuffer<glm::mat4> buffer_transformations;

    // view and camera
    glm::vec2 camera_rotation_angle_offset = glm::vec2(.0f, .0f);
    glm::vec2 camera_rotation_angle = glm::vec2(.0f, .0f);
    glm::vec2 mouse_start_location = glm::vec2(0.0f);
    glm::vec3 camera_position = CAMERA_START;
    glm::vec3 camera_look = glm::vec3(0.f, 0.f, -1.f);
    bool is_mouse_pressed = false;
    glm::mat4 projection = glm::perspective(45.0f, 1.0f * 1280 / 720, 0.1f, 10.0f);
    glm::mat4 view = glm::mat4(1.f);

    // scene
    std::map<std::string, size_t> object_names;
    std::vector<OpenGLPrimitives::ObjectInfo> objects;
    bool is_prepared = false;
};

}  // namespace gathering
