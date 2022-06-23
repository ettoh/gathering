#include "opengl_widget.hpp"

#include <algorithm>
#include <iostream>

#include "gathering/glm_include.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "opengl_toolkit.hpp"
#include "particle.hpp"

namespace gathering {

using OpenGLPrimitives::GLBuffer;
using OpenGLPrimitives::ObjectInfo;
using OpenGLPrimitives::VertexData;
using namespace tools;

// ------------------------------------------------------------------------------------------------

OpenGLWidget::OpenGLWidget() { init(); }

// ------------------------------------------------------------------------------------------------

OpenGLWidget::~OpenGLWidget() { destroy(); }

// ------------------------------------------------------------------------------------------------

void OpenGLWidget::init() {
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return;

    const char* glsl_version = "#version 460";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);  // anti aliasing

    // Create window with graphics context
    window = glfwCreateWindow(1920, 1080, "Gathering", NULL, NULL);  // TODO parameter
    if (window == NULL) return;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);  // vsync
    initEventHandler();   // glfw events like mouse/keyboard/window inputs

    bool err = gladLoadGL() == 0;
    if (err) {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_PRIMITIVE_RESTART);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(1.5f);
    glPrimitiveRestartIndex(MAX_ELEMENT_ID);

    // OpenGL-Version
    std::cout << "Open GL 4.6 needed. Given: ";
    std::cout << glGetString(GL_VERSION) << std::endl;

    // Read shader programs from files
    GLuint vertex_shader = createShader("shader/basic.vert", GL_VERTEX_SHADER);
    GLuint vertex_static_shader = createShader("shader/static.vert", GL_VERTEX_SHADER);
    GLuint frag_shader = createShader("shader/shaded.frag", GL_FRAGMENT_SHADER);
    GLuint frag_non_shaded = createShader("shader/non_shaded.frag", GL_FRAGMENT_SHADER);
    mvp_prog = createProgram(vertex_shader, frag_shader);
    mvp_prog_non_shaded = createProgram(vertex_shader, frag_non_shaded);
    static_prog = createProgram(vertex_static_shader, frag_shader);

    // bind uniform vbo to programs
    glGenBuffers(1, &vbo_uniforms);
    GLuint index = glGetUniformBlockIndex(mvp_prog, "Global");
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, vbo_uniforms);
    glUniformBlockBinding(mvp_prog, index, 1);
    index = glGetUniformBlockIndex(mvp_prog_non_shaded, "Global");
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, vbo_uniforms);
    glUniformBlockBinding(mvp_prog_non_shaded, index, 1);
    index = glGetUniformBlockIndex(static_prog, "Global");
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, vbo_uniforms);
    glUniformBlockBinding(static_prog, index, 1);

    // create storage buffer
    glGenBuffers(1, &vbo_static);
    glGenBuffers(1, &ibo_static);
    buffer_transformations = GLBuffer<glm::mat4>(GL_DYNAMIC_DRAW);
    buffer_transformations.gen();

    // create vao
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_static);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_static);
    glEnableVertexAttribArray(0);  // vertices
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), 0);
    glEnableVertexAttribArray(1);  // colors
    glVertexAttribPointer(
        1, 4, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)(sizeof(GL_FLOAT) * 3));
    glEnableVertexAttribArray(2);  // texture
    glVertexAttribPointer(
        2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)(sizeof(GL_FLOAT) * 7));
    glEnableVertexAttribArray(3);  // normals
    glVertexAttribPointer(
        3, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)(sizeof(GL_FLOAT) * 9));
    glBindBuffer(GL_ARRAY_BUFFER,
                 buffer_transformations.buffer_idx);  // object transformations
    glEnableVertexAttribArray(4);
    glEnableVertexAttribArray(5);
    glEnableVertexAttribArray(6);
    glEnableVertexAttribArray(7);
    // maximum size for vertexAttr is 4. So we split the 4x4matrix into 4x vec4
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 16, (void*)(0));
    glVertexAttribPointer(
        5, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 16, (void*)(sizeof(float) * 4));
    glVertexAttribPointer(
        6, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 16, (void*)(sizeof(float) * 8));
    glVertexAttribPointer(
        7, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 16, (void*)(sizeof(float) * 12));
    glVertexAttribDivisor(
        4, 1);  // update transformation attr. every 1 instance instead of every vertex
    glVertexAttribDivisor(5, 1);
    glVertexAttribDivisor(6, 1);
    glVertexAttribDivisor(7, 1);
    glBindVertexArray(0);
}

// ------------------------------------------------------------------------------------------------

void OpenGLWidget::setImageMode(bool image_mode) {
    is_image_mode = image_mode;
    auto ret = object_names.find("vessel");
    if (ret != object_names.end()) {
        objects[ret->second].enabled = !image_mode;
    }

    ret = object_names.find("particles");
    if (ret != object_names.end()) {
        objects[ret->second].gl_program = image_mode ? mvp_prog_non_shaded : mvp_prog;
    }

    clear_color = image_mode ? glm::vec3(0) : glm::vec3(0.09f);
}

// ------------------------------------------------------------------------------------------------

void OpenGLWidget::initEventHandler() {
    glfwSetWindowUserPointer(window, this);

    // Resize event
    glfwSetWindowSizeCallback(window, [](GLFWwindow* window, int width, int height) {
        glViewport(0, 0, width, height);
        window;  // to avoid compiler warning (unused formal parameter)
    });

    // Mouse click event
    glfwSetMouseButtonCallback(
        window, [](GLFWwindow* window, int button, int action, int mods) {
            mods;  // to avoid compiler warning (unused formal parameter)
            OpenGLWidget* p = (OpenGLWidget*)glfwGetWindowUserPointer(window);

            // ignore every input except left mouse button
            if (button != GLFW_MOUSE_BUTTON_LEFT) {
                return;
            }

            // if clicked inside the visualization (except UI), begin camera movement - if
            // mouse button released (no matter where), stop it
            switch (action) {
                case GLFW_PRESS:
                    if (glfwGetWindowAttrib(window, GLFW_HOVERED) &&
                        !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) &&
                        !ImGui::IsAnyItemHovered()) {
                        p->is_mouse_pressed = true;
                        double xpos, ypos;
                        glfwGetCursorPos(window, &xpos, &ypos);
                        p->mouse_start_location =
                            glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos));
                    }
                    break;
                case GLFW_RELEASE:
                    p->camera_rotation_angle += p->camera_rotation_angle_offset;
                    p->camera_rotation_angle_offset = glm::vec2(0.f);
                    p->camera_rotation_angle.x =
                        std::fmod(p->camera_rotation_angle.x, static_cast<float>(M_PI) * 2.f);
                    p->is_mouse_pressed = false;
                    break;
                default:
                    break;
            }
        });

    // Mouse move event
    glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xpos, double ypos) {
        OpenGLWidget* p = (OpenGLWidget*)glfwGetWindowUserPointer(window);

        if (p->is_mouse_pressed) {
            glm::vec2 diff = glm::vec2(xpos, ypos) - p->mouse_start_location;
            int screen_width, screen_height;
            glfwGetWindowSize(window, &screen_width, &screen_height);
            // moving the mouse along half the screen size rotates the scene by 90 degrees
            glm::vec2 angle = (static_cast<float>(M_PI) / 2.f) * diff /
                              glm::vec2(screen_width / 2, -screen_height / 2);
            // as long as the mouse button is not released, the changed rotation is applied as
            // an offset
            p->camera_rotation_angle_offset = angle;
        }
    });
}

// ------------------------------------------------------------------------------------------------

void OpenGLWidget::renderFrame() {
#ifdef GATHERING_AUTO_HEADLESS
    // do nothing if the glfw window is not visible
    if (!window_visible) return;
#endif

    if (glfwWindowShouldClose(window)) {
        // if the window is about to close - just don't! we may need the glfw context for
        // headless computing or later if the user wants the graphical output back. Just
        // pretend it is closed by hiding it... no one will notice ;)
        setWindowVisibility(false);
        glfwSetWindowShouldClose(window, GLFW_FALSE);
        return;
    }

    glfwPollEvents();
    buildGUI();  // Update gui

    // Prepare rendering
    ImGui::Render();  // State change
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // render new frame
    renderScene();
    if (!is_image_mode) ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // swap
    glfwSwapBuffers(window);
}

// ------------------------------------------------------------------------------------------------

void OpenGLWidget::renderScene() {
    // for every program (key) a vector with objects is stored
    for (const auto& obj : objects) {
        glUseProgram(obj.gl_program);
        glBindVertexArray(obj.gl_vao);

        if (obj.enabled) {
            if (obj.drawInstanced) {  // instanced rendering
                glDrawElementsInstancedBaseVertexBaseInstance(
                    obj.gl_draw_mode,
                    static_cast<GLint>(obj.number_elements),
                    obj.gl_element_type,
                    (void*)(obj.offset_elements),
                    static_cast<GLsizei>(obj.number_instances),
                    static_cast<GLint>(obj.base_index),
                    static_cast<GLint>(obj.base_instance));
            } else if (obj.number_elements == 0) {  // direct rendering with vertices
                glDrawArrays(obj.gl_draw_mode,
                             static_cast<GLint>(obj.offset_vertices),
                             static_cast<GLsizei>(obj.number_vertices));
            } else {  // direct rendering with elements
                glDrawElementsBaseVertex(obj.gl_draw_mode,
                                         static_cast<GLint>(obj.number_elements),
                                         obj.gl_element_type,
                                         (void*)(obj.offset_elements),
                                         static_cast<GLint>(obj.base_index));
            }
        }
    }

    glBindVertexArray(0);
}

// ------------------------------------------------------------------------------------------------

void OpenGLWidget::updateScene(const SceneData& scene) {
#ifdef GATHERING_AUTO_HEADLESS
    // do nothing if the glfw window is not visible
    if (!window_visible) return;
#endif

    // camera movement with keyboard
    glm::vec3 camera_movement = glm::vec3(0);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera_movement += glm::vec3(0, 0, -1);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera_movement += glm::vec3(-1, 0, 0);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera_movement += glm::vec3(0, 0, 1);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera_movement += glm::vec3(1, 0, 0);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera_movement += glm::vec3(0, 1, 0);
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        camera_movement += glm::vec3(0, -1, 0);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) camera_movement *= 2.f;

    // camera rotation
    glm::vec2 delta = camera_rotation_angle + camera_rotation_angle_offset;
    // the camera can not be circumnavigated along the poles
    float max_angle_y = static_cast<float>(M_PI) / 2.f - 0.1f;
    delta.y = glm::clamp(delta.y, -max_angle_y, max_angle_y);
    camera_rotation_angle.y = glm::clamp(camera_rotation_angle.y, -max_angle_y, max_angle_y);

    // time since last frame
    float dt = stop_watch.round() * 0.000001f;

    // calc view matrix
    glm::mat4 camera_rotation = glm::rotate(glm::mat4(1), delta.y, glm::vec3(1.f, 0.f, 0.f));
    glm::mat4 world_rotation = glm::rotate(glm::mat4(1), delta.x, glm::vec3(0.f, -1.f, 0.f));
    glm::mat4 total_rotation = world_rotation * camera_rotation;
    glm::vec4 look_direction = total_rotation * glm::vec4(camera_look, 1.f);
    camera_position += glm::vec3(total_rotation * glm::vec4(camera_movement, 1.f) * dt * 3.f);
    if (!is_image_mode) {
        view = glm::lookAt(camera_position,
                           camera_position + glm::vec3(look_direction),
                           glm::vec3(0.0f, 1.0f, 0.0f));  // (position, look at, up)
    }

    // Calc MVP
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);  // viewport: x, y, width, height

    // projection matrix
    if (!is_image_mode) {
        projection = glm::perspective(45.0f, 1.0f * viewport[2] / viewport[3], 0.01f, 100.0f);
    }

    // push mvp to VBO
    glBindBuffer(GL_UNIFORM_BUFFER, vbo_uniforms);
    glBufferData(
        GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4) + sizeof(glm::vec3), 0, GL_DYNAMIC_DRAW);
    glBufferSubData(
        GL_UNIFORM_BUFFER, 0 * sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(view));
    glBufferSubData(GL_UNIFORM_BUFFER,
                    1 * sizeof(glm::mat4),
                    sizeof(glm::mat4),
                    glm::value_ptr(projection));
    glBufferSubData(GL_UNIFORM_BUFFER,
                    2 * sizeof(glm::mat4),
                    sizeof(glm::vec3),
                    glm::value_ptr(camera_position));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // #############################
    // # dynamic part of scene
    // #############################

    buffer_transformations.values.clear();

    auto info_particles = getObjectInfo("particles");
    if (info_particles != nullptr)
        info_particles->base_instance = buffer_transformations.size();  // offset
    for (const auto& particle : scene.particles) {
        glm::mat4 mat = glm::translate(particle.position);
        buffer_transformations.values.push_back(mat);
    }

    // push data to VBO
    if (buffer_transformations.values.size() != 0) {
        size_t size_transformation =
            sizeof(buffer_transformations.values[0]) * buffer_transformations.values.size();
        glBindBuffer(GL_ARRAY_BUFFER, buffer_transformations.buffer_idx);  // set active
        glBufferData(GL_ARRAY_BUFFER,
                     size_transformation,
                     &buffer_transformations.values[0],
                     GL_STREAM_DRAW);  // push data
    }
}

// ------------------------------------------------------------------------------------------------

void OpenGLWidget::prepareInstance(SceneData& scene) {
    deleteInstance();
    std::vector<OpenGLPrimitives::Object> raw_objects;

    // particles
    OpenGLPrimitives::Object particles = OpenGLPrimitives::createSphere(
        float(RADIUS_PARTICLE), glm::vec3(0.f), 5, glm::vec4(1, 1, 1, 1));
    particles.name = "particles";
    particles.drawInstanced = true;
    particles.instance_count = scene.particles.size();
    particles.gl_vao = vao;
    particles.gl_program = mvp_prog;
    raw_objects.push_back(particles);

    // vessel
    scene.vessel.gl_draw_mode = GL_TRIANGLES;
    scene.vessel.gl_vao = vao;
    scene.vessel.name = "vessel";
    scene.vessel.gl_program = static_prog;
    raw_objects.push_back(scene.vessel);

    // sort raw_objects by their VAO/program in order to reduce sate changes
    pushStaticSceneToGPU(raw_objects);
    std::sort(raw_objects.begin(), raw_objects.end());

    // build map to find raw_objects by name
    for (int i = 0; i < raw_objects.size(); i++) {
        const auto& obj = raw_objects[i];
        if (obj.name != "") {
            auto res = object_names.insert({obj.name, i});
            if (!res.second) {
                printf("Insertion of object '%s' failed.\n", obj.name.c_str());
                assert(false);
                exit(EXIT_FAILURE);
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------

void OpenGLWidget::pushStaticSceneToGPU(
    const std::vector<OpenGLPrimitives::Object>& scene_objects) {
    objects.clear();
    size_t vertex_size = 0;
    size_t element_size = 0;

    for (const auto& object : scene_objects) {
        vertex_size += object.totalVertexSize();
        element_size += object.totalElementSize();
    }

    // allocate memory on gpu
    glBindBuffer(GL_ARRAY_BUFFER, vbo_static);                               // set active
    glBufferData(GL_ARRAY_BUFFER, vertex_size, 0, GL_STATIC_DRAW);           // allocate memory
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_static);                       // set active
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, element_size, 0, GL_STATIC_DRAW);  // allocate memory

    // push data to GPU
    size_t offset_vertices = 0;
    size_t offset_elements = 0;
    size_t vertex_count = 0;

    for (const auto& object : scene_objects) {
        ObjectInfo info(object);
        info.base_index = vertex_count;
        info.offset_elements = offset_elements;
        info.offset_vertices = vertex_count;
        objects.push_back(info);

        size_t object_size = object.totalVertexSize();  // size of all vertices in byte

        if (object_size != 0) {
            // all static vertex data
            glBindBuffer(GL_ARRAY_BUFFER, vbo_static);
            glBufferSubData(
                GL_ARRAY_BUFFER, offset_vertices, object_size, &object.vertices[0]);
            offset_vertices += object_size;
        }

        // if object is defined by vertex id's, because multiple triangles uses the same
        // vertices
        if (object.isElementObject()) {
            size_t object_element_size =
                object.totalElementSize();  // size of all vertex id's in byte
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_static);

            switch (object.gl_element_type) {
                case GL_UNSIGNED_SHORT:
                    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER,
                                    offset_elements,
                                    object_element_size,
                                    &object.elements_16()[0]);
                    break;
                case GL_UNSIGNED_BYTE:
                    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER,
                                    offset_elements,
                                    object_element_size,
                                    &object.elements_8()[0]);
                    break;
                default:
                    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER,
                                    offset_elements,
                                    object_element_size,
                                    &object.elements[0]);
                    break;
            }

            offset_elements += object_element_size;
        }
        vertex_count += object.vertexCount();
    }
}

// ------------------------------------------------------------------------------------------------

ObjectInfo* OpenGLWidget::getObjectInfo(const std::string& name) {
    auto result = object_names.find(name);
    if (result != object_names.end()) {
        if (result->second < objects.size()) return &objects[result->second];
    }

    printf("There is no object with the name '%s'!\n", name.c_str());
    return nullptr;
}

// ------------------------------------------------------------------------------------------------

void OpenGLWidget::buildGUI() {
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Simulation control panel
    {
        ImGui::SetNextWindowSizeConstraints(ImVec2(340, 180), ImVec2(1500, 1500));
        ImGui::Begin("Simulation control panel");  // Create a window and append into it.

        ImGui::PushItemWidth(ImGui::GetFontSize() * -12);
        ImGui::SameLine();
        if (ImGui::Button("Reset camera")) {
            camera_rotation_angle = glm::vec2(0.f);
            camera_position = CAMERA_START;
        }

        if (ImGui::CollapsingHeader("Settings", ImGuiTreeNodeFlags_None)) {
            static bool hide_earth = false;
            if (ImGui::Checkbox("Hide earth", &hide_earth)) {
                auto info = getObjectInfo("central_mass");
                if (info != nullptr) info->enabled = !hide_earth;
            }
        }

        ImGui::Text("x = %.3f; y = %.3f; z = %.3f",
                    camera_position.x,
                    camera_position.y,
                    camera_position.z);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                    1000.0f / ImGui::GetIO().Framerate,
                    ImGui::GetIO().Framerate);
        ImGui::End();
    }
}

// ------------------------------------------------------------------------------------------------

void OpenGLWidget::destroy() {
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    deleteInstance();

    glDeleteProgram(mvp_prog);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &ibo_static);
    glDeleteBuffers(1, &vbo_static);
    glDeleteBuffers(1, &vbo_uniforms);
    glfwDestroyWindow(window);
    glfwTerminate();
}

// ------------------------------------------------------------------------------------------------

void OpenGLWidget::deleteInstance() {
    objects.clear();
    object_names.clear();
}

// ------------------------------------------------------------------------------------------------

void OpenGLWidget::setWindowVisibility(const bool is_visible) {
    if (is_visible) {
        glfwShowWindow(window);
        window_visible = true;
    } else {
        glfwHideWindow(window);
        window_visible = false;
    }
}

}  // namespace gathering
