#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"
#include "stb_image.h"
#include "tinyobj_loader_c.h"

#include "example.h"
#include "application.h"

static const int WINDOW_WIDTH = 800;
static const int WINDOW_HEIGHT = 600;

static const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

static const char *validation_layer_names[] = {"VK_LAYER_LUNARG_standard_validation"};
static const uint32_t validation_layer_count = sizeof(validation_layer_names) / sizeof(const char *);

static const char *device_extension_names[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
static uint32_t device_extension_count = sizeof(device_extension_names) / sizeof(const char *);

static const char *MODEL_SRC_PATH = "resources\\chalet.obj";
static const char *MODEL_BIN_PATH = "resources\\chalet.bin";
static const char *TEXTURE_PATH = "resources\\chalet.jpg";
static const char *VERTEX_SHADER_PATH = "resources\\vert.spv";
static const char *FRAGMENT_SHADER_PATH = "resources\\frag.spv";

typedef struct extension_functions {
    PFN_vkCreateDebugReportCallbackEXT f_vkCreateDebugReportCallbackEXT;
    PFN_vkDestroyDebugReportCallbackEXT f_vkDestroyDebugReportCallbackEXT;
} extension_functions;

typedef struct swap_chain_details {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR *formats;
    uint32_t format_count;
    VkPresentModeKHR *present_modes;
    uint32_t present_mode_count;
} swap_chain_details;

typedef struct vertex {
    vec3 position;
    vec2 texcoord;
    vec3 color;
} vertex;

//const vertex vertices[8] = {
//    {{-0.5f, -0.5f,  0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
//    {{ 0.5f, -0.5f,  0.0f}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
//    {{ 0.5f,  0.5f,  0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
//    {{-0.5f,  0.5f,  0.0f}, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
//
//    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
//    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
//    {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
//    {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}}
//};
//
//const uint32_t indices[12] = {
//    0, 1, 2, 2, 3, 0,
//    4, 5, 6, 6, 7, 4
//};

typedef struct uniform_buffer_object {
    mat4 model;
    mat4 view;
    mat4 proj;
} uniform_buffer_object;

struct my_application {
    // glfw objects
    GLFWwindow *window;

    // vulkan objects
    VkInstance instance;
    VkSurfaceKHR surface;
    VkDebugReportCallbackEXT debug_callback;
    VkPhysicalDevice physical_device;
    VkDevice device;
    int32_t graphics_family; // index of queue family which contain VK_QUEUE_GRAPHICS_BIT flag
    int32_t present_family; // index of queue family which contain platfrom presentation flag
    VkQueue graphics_queue;
    VkQueue present_queue;
    VkSwapchainKHR swap_chain;
    VkImage *swap_chain_images;
    uint32_t swap_chain_image_count;
    VkFormat swap_chain_image_format;
    VkExtent2D swap_chain_extent;
    VkImageView *swap_chain_image_views;
    VkRenderPass render_pass;
    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet *descriptor_sets;
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
    VkFramebuffer *swap_chain_frame_buffers;
    VkCommandPool command_pool;
    VkCommandBuffer *command_buffers;
    VkSemaphore *image_available_semaphores;
    VkSemaphore *render_finished_semaphores;
    VkFence *flight_fences;

    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_buffer_memory;
    VkBuffer index_buffer;
    VkDeviceMemory index_buffer_memory;
    VkBuffer *uniform_buffers;
    VkDeviceMemory *uniform_buffer_memories;
    uint32_t mip_levels;
    VkImage texture_image;
    VkDeviceMemory texture_image_memory;
    VkImageView texture_image_view;
    VkSampler texture_sampler;

    VkFormat depth_format;
    VkImage depth_image;
    VkDeviceMemory depth_image_memory;
    VkImageView depth_image_view;

    VkSampleCountFlagBits msaa_samplers;
    VkImage color_image;
    VkDeviceMemory color_image_memory;
    VkImageView color_image_view;

    // model
    vertex *vertices;
    uint32_t vertex_count;
    uint32_t *indices;
    uint32_t index_count;

    // function pointer
    extension_functions *ext_funcs;

    uint32_t current_frame;
    bool frame_buffer_resized;
};

extern my_application * constructor(my_application *self);
extern void destructor(my_application *self);

extern void init_window(my_application *self);
extern bool init_vulkan(my_application *self);
extern void main_loop(my_application *self);
extern void cleanup(my_application *self);

extern void load_extension_functions(my_application *self);
extern void unload_extension_functions(my_application *self);
extern bool check_validation_layer_support(my_application *self);
extern bool create_instance(my_application *self);
extern bool check_instance_extension_support(my_application *self, const char **ext_names, uint32_t ext_name_count);
extern bool pick_physical_device(my_application *self);
extern bool is_physical_device_suitable(my_application *self, VkPhysicalDevice physical_device);
extern bool check_physical_device_extension_support(my_application *self, VkPhysicalDevice physical_device);
extern bool find_queue_families(my_application *self, VkPhysicalDevice physical_device);
extern bool create_logic_device(my_application *self);

extern bool create_swap_chain(my_application *self);
extern bool create_swap_chain_image_views(my_application *self);
extern bool create_render_pass(my_application *self);
extern bool create_graphics_pipeline(my_application *self);
extern bool create_frame_buffers(my_application *self);
extern VkShaderModule create_shader_module(my_application *self, void *shader_code, uint32_t length);
extern bool create_command_pool(my_application *self);
extern bool create_command_buffers(my_application *self);
extern bool create_sync_objects(my_application *self);
extern int32_t find_memory_type(my_application *self, uint32_t type_filter, VkMemoryPropertyFlags flags);
extern bool create_vertex_buffer(my_application *self);
extern bool create_index_buffer(my_application *self);
extern bool create_descriptor_set_layout(my_application *self);
extern bool create_uniform_buffers(my_application *self);
extern bool create_descriptor_pool(my_application *self);
extern bool create_descriptor_set(my_application *self);
extern bool create_texture_image(my_application *self);
extern bool create_texture_image_view(my_application *self);
extern bool create_texture_sampler(my_application *self);
extern bool create_depth_resources(my_application *self);
extern bool load_model_source(my_application *self);
extern bool load_model_binary(my_application *self);
extern bool create_color_resources(my_application *self);

extern void cleanup_swap_chain(my_application *self);
extern void recreate_swap_chain(my_application *self);

extern void draw_frame(my_application *self);
extern void update_uniform_buffer(my_application *self, uint32_t index);

// utilities
extern bool read_file(const char *file_name, void **content, uint32_t *length);
extern bool create_buffer(my_application *self, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags property, VkBuffer *buffer, VkDeviceMemory *buffer_mempry);
extern VkCommandBuffer begin_single_time_commands(my_application *self);
extern void end_single_time_commands(my_application *self, VkCommandBuffer command_buffer);
extern bool copy_buffer(my_application *self, VkBuffer src, VkBuffer dst, VkDeviceSize size);
extern bool create_image_2d(my_application *self, uint32_t width, uint32_t height, uint32_t mip_levels, VkSampleCountFlagBits samplers, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage *image, VkDeviceMemory *image_memory);
extern void transition_image_layout(my_application *self, VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout, uint32_t mip_levels);
extern bool generate_mipmaps(my_application *self, VkImage image, VkFormat format, int32_t width, int32_t height, uint32_t mip_levels);
extern void copy_buffer_to_image(my_application *self, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
extern VkImageView create_image_view_2d(my_application *self, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_levels);
extern VkFormat find_supported_format(my_application *self, VkFormat *formats, uint32_t count, VkImageTiling tiling, VkFormatFeatureFlags features);
extern bool has_stencil_component(VkFormat format);
extern VkFormat find_depth_format(my_application *self);
extern VkSampleCountFlagBits get_max_usable_sample_count(my_application *self);

// public

my_application * my_application_new(void) {
    my_application *app = calloc(1, sizeof(my_application));
    return constructor(app);
}

void my_application_delete(my_application *self) {
    destructor(self);
}

void my_application_run(my_application *self) {
    init_window(self);
    if (init_vulkan(self)) {
        main_loop(self);
    }
    cleanup(self);
}

// private 

static my_application * constructor(my_application *self) {
    if (self) {
        // init self here
        self->graphics_family = -1;
        self->present_family = -1;
        self->frame_buffer_resized = false;
        self->msaa_samplers = VK_SAMPLE_COUNT_1_BIT;
    }
    return self;
}

static void destructor(my_application *self) {
    free(self);
}

static void frame_buffer_resize_callback(GLFWwindow *window, int width,int height) {
    my_application *self = glfwGetWindowUserPointer(window);
    if (self) {
        self->frame_buffer_resized = true;
    }
}

static void init_window(my_application *self) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan Example", NULL, NULL);
    glfwSetWindowUserPointer(window, self);
    glfwSetFramebufferSizeCallback(window, frame_buffer_resize_callback);
    self->window = window;
}

static bool init_vulkan(my_application *self) {
    bool ret = false;

    do {

#ifdef ENABLE_VALIDATION_LAYERS
        if (!check_validation_layer_support(self)) { break; }
#endif
        if (!create_instance(self)) { break; }
        if (!pick_physical_device(self)) { break; }
        if (!create_logic_device(self)) { break; }
        if (!create_swap_chain(self)) { break; }
        if (!create_swap_chain_image_views(self)) { break; }
        if (!create_render_pass(self)) { break; }
        if (!create_descriptor_set_layout(self)) { break; }
        if (!create_graphics_pipeline(self)) { break; }
        if (!create_command_pool(self)) { break; }
        if (!create_color_resources(self)) { break; }
        if (!create_depth_resources(self)) { break; }
        if (!create_frame_buffers(self)) { break; }
        if (!create_texture_image(self)) { break; }
        if (!create_texture_image_view(self)) { break; }
        if (!create_texture_sampler(self)) { break; }
        if (!load_model_binary(self)) { break; }
        if (!create_vertex_buffer(self)) { break; }
        if (!create_index_buffer(self)) { break; }
        if (!create_uniform_buffers(self)) { break; }
        if (!create_descriptor_pool(self)) { break; }
        if (!create_descriptor_set(self)) { break; }
        if (!create_command_buffers(self)) { break; }
        if (!create_sync_objects(self)) { break; }

        ret = true;

    } while (false);

    return ret;
}

static void main_loop(my_application *self) {
    while (!glfwWindowShouldClose(self->window)) {
        glfwPollEvents();
        draw_frame(self);
    }

    vkDeviceWaitIdle(self->device);
}

static void cleanup(my_application *self) {
    cleanup_swap_chain(self);

    if (self->descriptor_pool) {
        vkDestroyDescriptorPool(self->device, self->descriptor_pool, MY_VK_ALLOCATOR);
    }

    if (self->descriptor_sets) {
        free(self->descriptor_sets);
    }

    if (self->descriptor_set_layout) {
        vkDestroyDescriptorSetLayout(self->device, self->descriptor_set_layout, MY_VK_ALLOCATOR);
    }

    if (self->vertices) {
        free(self->vertices);
    }

    if (self->indices) {
        free(self->indices);
    }

    if (self->uniform_buffers && self->uniform_buffer_memories) {
        for (uint32_t i = 0; i < self->swap_chain_image_count; ++i) {
            vkDestroyBuffer(self->device, self->uniform_buffers[i], MY_VK_ALLOCATOR);
            vkFreeMemory(self->device, self->uniform_buffer_memories[i], MY_VK_ALLOCATOR);
        }
        free(self->uniform_buffers);
        free(self->uniform_buffer_memories);
    }

    if (self->index_buffer) {
        vkDestroyBuffer(self->device, self->index_buffer, MY_VK_ALLOCATOR);
    }

    if (self->index_buffer_memory) {
        vkFreeMemory(self->device, self->index_buffer_memory, MY_VK_ALLOCATOR);
    }

    if (self->vertex_buffer) {
        vkDestroyBuffer(self->device, self->vertex_buffer, MY_VK_ALLOCATOR);
    }

    if (self->vertex_buffer_memory) {
        vkFreeMemory(self->device, self->vertex_buffer_memory, MY_VK_ALLOCATOR);
    }

    if (self->texture_sampler) {
        vkDestroySampler(self->device, self->texture_sampler, MY_VK_ALLOCATOR);
    }

    if (self->texture_image_view) {
        vkDestroyImageView(self->device, self->texture_image_view, MY_VK_ALLOCATOR);
    }

    if (self->texture_image) {
        vkDestroyImage(self->device, self->texture_image, MY_VK_ALLOCATOR);
    }

    if (self->texture_image_memory) {
        vkFreeMemory(self->device, self->texture_image_memory, MY_VK_ALLOCATOR);
    }

    if (self->image_available_semaphores) {
        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            vkDestroySemaphore(self->device, self->image_available_semaphores[i], MY_VK_ALLOCATOR);
        }
        free(self->image_available_semaphores);
    }

    if (self->render_finished_semaphores) {
        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            vkDestroySemaphore(self->device, self->render_finished_semaphores[i], MY_VK_ALLOCATOR);
        }
        free(self->render_finished_semaphores);
    }

    if (self->flight_fences) {
        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            vkDestroyFence(self->device, self->flight_fences[i], MY_VK_ALLOCATOR);
        }
        free(self->flight_fences);
    }

    if (self->command_pool) {
        vkDestroyCommandPool(self->device, self->command_pool, MY_VK_ALLOCATOR);
    }

#ifdef ENABLE_VALIDATION_LAYERS
    if (self->debug_callback) {
        self->ext_funcs->f_vkDestroyDebugReportCallbackEXT(self->instance, self->debug_callback, MY_VK_ALLOCATOR);
    }
#endif

    if (self->device) {
        vkDestroyDevice(self->device, MY_VK_ALLOCATOR);
    }

    if (self->surface) {
        vkDestroySurfaceKHR(self->instance, self->surface, MY_VK_ALLOCATOR);
    }

    unload_extension_functions(self);

    if (self->instance) {
        vkDestroyInstance(self->instance, MY_VK_ALLOCATOR);
    }

    if (self->window) {
        glfwDestroyWindow(self->window);
    }

    glfwTerminate();
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_layer_callback(
    VkDebugReportFlagsEXT flag,
    VkDebugReportObjectTypeEXT obj_type,
    uint64_t obj,
    size_t location,
    int32_t code,
    const char *layer_prefix,
    const char *message,
    void *user_data) {

    LOG("Validation layer: %s\n", message);
    return VK_FALSE;
}

static void load_extension_functions(my_application *self) {
    VkInstance instance = self->instance;
    if (!instance) {
        return;
    }

    free(self->ext_funcs);
    extension_functions *ext_funcs = malloc(sizeof(extension_functions));

    ext_funcs->f_vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
    ext_funcs->f_vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");

    self->ext_funcs = ext_funcs;
}

static void unload_extension_functions(my_application *self) {
    free(self->ext_funcs);
    self->ext_funcs = NULL;
}

static bool check_validation_layer_support(my_application *self) {
    uint32_t layer_count = 0;
    vkEnumerateInstanceLayerProperties(&layer_count, NULL);

    if (layer_count == 0) {
        LOG("No Validation Layer Found!\n");
        return false;
    }

    VkLayerProperties *layers = malloc(layer_count * sizeof(VkLayerProperties));
    vkEnumerateInstanceLayerProperties(&layer_count, layers);

    bool all_found = true;
    for (uint32_t i = 0; i < validation_layer_count; ++i) {
        bool is_found = false;
        for (uint32_t j = 0; j < layer_count; ++j) {
            if (!strcmp(validation_layer_names[i], layers[j].layerName)) {
                is_found = true;
                break;
            }
        }

        if (!is_found) {
            LOG("Validation layer %s is not found!\n", validation_layer_names[i]);
            all_found = false;
            break;
        } else {
            LOG("Validation layer %s is found!\n", validation_layer_names[i]);
        }
    }

    free(layers);
    return all_found;
}

static bool create_instance(my_application *self) {
    bool ret = true;

    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = "Hello Vulkan",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0
    };

    uint32_t glfw_ext_count = 0;
    const char **glfw_exts = glfwGetRequiredInstanceExtensions(&glfw_ext_count);

    uint32_t ext_name_count = glfw_ext_count;
    const char **ext_names = glfw_exts;
#ifdef ENABLE_VALIDATION_LAYERS
    ++ ext_name_count;
    ext_names = malloc(ext_name_count * sizeof(const char *));
    memcpy((void *)ext_names, glfw_exts, glfw_ext_count * sizeof(const char *));
    ext_names[ext_name_count - 1] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
#endif

    ret = check_instance_extension_support(self, ext_names, ext_name_count);

    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        //.flags = 0,
        .pApplicationInfo = &app_info,
#ifdef ENABLE_VALIDATION_LAYERS
        .enabledLayerCount = validation_layer_count,
        .ppEnabledLayerNames = validation_layer_names,
#else
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
#endif
        .enabledExtensionCount = ext_name_count,
        .ppEnabledExtensionNames = ext_names
    };

    VkResult result = vkCreateInstance(&create_info, MY_VK_ALLOCATOR, &(self->instance));
    assert(result == VK_SUCCESS);
    ret = (ret ? result == VK_SUCCESS : ret);

    load_extension_functions(self);

////create surface object manually
//#ifdef WIN32
//    VkWin32SurfaceCreateInfoKHR surface_create_info = {
//        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
//        .pNext = NULL,
//        // VkWin32SurfaceCreateFlagsKHR    flags;
//        .hinstance = GetModuleHandle(NULL),
//        .hwnd = glfwGetWin32Window(self->window)
//    };
//
//    result = vkCreateWin32SurfaceKHR(self->instance, &surface_create_info, MY_VK_ALLOCATOR, &self->surface);
//    ret = (ret ? result == VK_SUCCESS : ret);
//#endif // WIN32

    result = glfwCreateWindowSurface(self->instance, self->window, MY_VK_ALLOCATOR, &self->surface);
    ret = (ret ? result == VK_SUCCESS : ret);

#ifdef ENABLE_VALIDATION_LAYERS
    free((void *)ext_names);

    // create debug callback
    VkDebugReportCallbackCreateInfoEXT debug_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
        .pNext = NULL,
        .flags = VK_DEBUG_REPORT_ERROR_BIT_EXT 
#if defined(_DEBUG) || defined(DEBUG)
//                 | VK_DEBUG_REPORT_DEBUG_BIT_EXT
#endif
                 | VK_DEBUG_REPORT_WARNING_BIT_EXT
                 | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
//                 | VK_DEBUG_REPORT_INFORMATION_BIT_EXT,
        .pfnCallback = debug_layer_callback,
        .pUserData = NULL
    };

    result = self->ext_funcs->f_vkCreateDebugReportCallbackEXT(self->instance, &debug_create_info, MY_VK_ALLOCATOR, &(self->debug_callback));
    assert(result == VK_SUCCESS);
    ret = (ret ? result == VK_SUCCESS : ret);
#endif

    return ret;
}

static bool check_instance_extension_support(my_application *self, const char **ext_names, uint32_t ext_name_count) {
    if (!ext_name_count) {
        return true;
    }

    if (ext_names == NULL) {
        return false;
    }

    uint32_t ext_count = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &ext_count, NULL);
    if (ext_count == 0) {
        LOG("No Instance extension found!\n");
        return false;
    }

    VkExtensionProperties *exts = malloc(ext_count * sizeof(VkExtensionProperties));
    vkEnumerateInstanceExtensionProperties(NULL, &ext_count, exts);

    bool ret = true;
    for (uint32_t i = 0; i < ext_name_count; ++i) {
        bool is_found = false;
        const char *ext_name = ext_names[i];
        for (uint32_t j = 0; j < ext_count; ++j) {
            if (!strcmp(ext_name, exts[j].extensionName)) {
                is_found = true;
                break;
            }
        }
        if (is_found) {
            LOG("Instance extension %s is found!\n", ext_names[i]);
        } else {
            LOG("Instance extension %s is not found!\n", ext_names[i]);
        }
        ret = ret && is_found;
    }

    free(exts);
    return ret;
}

static bool pick_physical_device(my_application *self) {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(self->instance, &device_count, NULL);

    if (device_count == 0) {
        LOG("No Physical device found!");
        return false;
    }

    VkPhysicalDevice *devices = malloc(device_count * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(self->instance, &device_count, devices);

    // check if device suitable
    for (uint32_t i = 0; i < device_count; ++i) {
        if (is_physical_device_suitable(self, devices[i])) {
            self->physical_device = devices[i];
            self->msaa_samplers = get_max_usable_sample_count(self);
            break;
        }
    }

    free(devices);

    if (!self->physical_device) {
        LOG("No suitable physical device found!\n");
    }

    return (self->physical_device != VK_NULL_HANDLE);
}

static bool is_physical_device_suitable(my_application *self, VkPhysicalDevice physical_device) {
    bool ret = false;

    VkPhysicalDeviceProperties device_properties;
    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceProperties(physical_device, &device_properties);
    vkGetPhysicalDeviceFeatures(physical_device, &device_features);

    do {
        if (device_properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            break;
        }

        if (device_features.samplerAnisotropy != VK_TRUE) {
            break;
        }

        if (device_features.tessellationShader != VK_TRUE) {
            break;
        }

        if (!check_physical_device_extension_support(self, physical_device)) {
            break;
        }

        if (!find_queue_families(self, physical_device)) {
            break;
        }

        ret = true;

    } while (false);

    
    return ret;
}

static bool check_physical_device_extension_support(my_application *self, VkPhysicalDevice physical_device) {
    uint32_t ext_count = 0;
    vkEnumerateDeviceExtensionProperties(physical_device, NULL, &ext_count, NULL);
    if (!ext_count) {
        LOG("No Physical device extension found!");
        return false;
    }

    VkExtensionProperties *exts = malloc(ext_count * sizeof(VkExtensionProperties));
    vkEnumerateDeviceExtensionProperties(physical_device, NULL, &ext_count, exts);

    bool ret = true;

    for (uint32_t i = 0; i < device_extension_count; ++i) {
        bool is_found = false;
        for (uint32_t j = 0; j < ext_count; ++j) {
            if (!strcmp(device_extension_names[i], exts[j].extensionName)) {
                is_found = true;
                break;
            }
        }
        if (is_found) {
            LOG("device extension %s is found!\n", device_extension_names[i]);
        } else {
            LOG("device extension %s is not found!\n", device_extension_names[i]);
            ret = false;
        }
    }

    free(exts);
    return ret;
}

static bool find_queue_families(my_application *self, VkPhysicalDevice physical_device) {
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, NULL);

    if (queue_family_count == 0) {
        LOG("No Queue family found!");
        return false;
    }

    VkQueueFamilyProperties *queue_families = malloc(queue_family_count * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families);

    for (uint32_t i = 0; i < queue_family_count; ++i) {
        if (queue_families[i].queueCount > 0 
            && queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            self->graphics_family = i;
            break;
        }
    }

    VkBool32 present_support = VK_FALSE;
    for (uint32_t i = 0; i < queue_family_count; ++i) {
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, self->surface, &present_support);
        if (present_support == VK_TRUE) {
            self->present_family = i;
            break;
        }
    }

    free(queue_families);

    return (self->graphics_family > -1 && self->present_family > -1);
}

static bool create_logic_device(my_application *self) {
    float priorities[] = {1.0f};
    VkDeviceQueueCreateInfo queue_create_info[2] = {
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = NULL,
    //        VkDeviceQueueCreateFlags flags,
            .queueFamilyIndex = self->graphics_family,
            .queueCount = 1,
            .pQueuePriorities = priorities
        }, {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = NULL,
    //        VkDeviceQueueCreateFlags flags,
            .queueFamilyIndex = self->present_family,
            .queueCount = 1,
            .pQueuePriorities = priorities
        }
    };

    VkPhysicalDeviceFeatures device_features = {VK_FALSE};
    device_features.samplerAnisotropy = VK_TRUE;
    //device_features.sampleRateShading = VK_TRUE;

    VkDeviceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
//        VkDeviceCreateFlags flags,
        .queueCreateInfoCount = (self->graphics_family == self->present_family ? 1 : 2),
        .pQueueCreateInfos = queue_create_info,
#ifdef ENABLE_VALIDATION_LAYERS
        .enabledLayerCount = validation_layer_count,
        .ppEnabledLayerNames = validation_layer_names,
#else
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
#endif
        .enabledExtensionCount = device_extension_count,
        .ppEnabledExtensionNames = device_extension_names,
        .pEnabledFeatures = &device_features
    };

    VkResult result = vkCreateDevice(self->physical_device, &create_info, MY_VK_ALLOCATOR, &(self->device));
    assert(result == VK_SUCCESS);

    vkGetDeviceQueue(self->device, self->graphics_family, 0, &(self->graphics_queue));
    assert(self->graphics_queue != VK_NULL_HANDLE);

    vkGetDeviceQueue(self->device, self->present_family, 0, &(self->present_queue));
    assert(self->present_queue != VK_NULL_HANDLE);

    return (result == VK_SUCCESS);
}

static bool create_swap_chain(my_application *self) {
    swap_chain_details details;
    details.formats = NULL;
    details.present_modes = NULL;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(self->physical_device, self->surface, &(details.capabilities));

    // formats
    vkGetPhysicalDeviceSurfaceFormatsKHR(self->physical_device, self->surface, &(details.format_count), NULL);
    if (details.format_count) {
        details.formats = malloc(details.format_count * sizeof(VkSurfaceFormatKHR));
        vkGetPhysicalDeviceSurfaceFormatsKHR(self->physical_device, self->surface, &(details.format_count), details.formats);
    }

    // present mode
    vkGetPhysicalDeviceSurfacePresentModesKHR(self->physical_device, self->surface, &(details.present_mode_count), NULL);
    if (details.present_mode_count) {
        details.present_modes = malloc(details.present_mode_count * sizeof(VkPresentModeKHR));
        vkGetPhysicalDeviceSurfacePresentModesKHR(self->physical_device, self->surface, &(details.present_mode_count), details.present_modes);
    }

    bool ret = false;

    do {
        if (details.format_count == 0 || details.present_mode_count == 0) {
            LOG("No surface format or present mode with current device.\n");
            break;
        }

        // choose surface format
        VkSurfaceFormatKHR surface_format = {VK_FORMAT_UNDEFINED, };
        if (details.format_count == 1 && details.formats[0].format == VK_FORMAT_UNDEFINED) {
            surface_format.format = VK_FORMAT_B8G8R8A8_UNORM;
            surface_format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        } else {
            for (uint32_t i = 0; i < details.format_count; ++i) {
                if (details.formats[i].format == VK_FORMAT_B8G8R8A8_UNORM
                    && details.formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    surface_format = details.formats[i];
                    break;
                }
            }
            if (surface_format.format == VK_FORMAT_UNDEFINED) {
                surface_format = details.formats[0];
            }
        }

        // choose present mode
        VkPresentModeKHR present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        for (uint32_t i = 0; i < details.present_mode_count; ++i) {
            if (details.present_modes[i] == VK_PRESENT_MODE_FIFO_KHR) {
                present_mode = VK_PRESENT_MODE_FIFO_KHR;
                break;
            }
        }
        for (uint32_t i = 0; i < details.present_mode_count; ++i) {
            if (details.present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
                present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
        }

        // choose swap extent
        VkExtent2D extent;
        if (details.capabilities.currentExtent.width != UINT32_MAX) {
            extent = details.capabilities.currentExtent;
        } else {
            int width, height;
            glfwGetFramebufferSize(self->window, &width, &height);
            extent.width = (uint32_t)width;
            extent.height = (uint32_t)height;
        }

        // setup image count
        uint32_t image_count = details.capabilities.minImageCount + 1;
        if (details.capabilities.maxImageCount > 0
            && image_count > details.capabilities.maxImageCount) {
            image_count = details.capabilities.maxImageCount;
        }

        // create swap chain
        VkSwapchainCreateInfoKHR createInfo = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext = NULL,
            // VkSwapchainCreateFlagsKHR        flags;
            .surface = self->surface,
            .minImageCount = image_count,
            .imageFormat = surface_format.format,
            .imageColorSpace = surface_format.colorSpace,
            .imageExtent = extent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .preTransform = details.capabilities.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = present_mode,
            .clipped = VK_TRUE,
            .oldSwapchain = VK_NULL_HANDLE
        };

        uint32_t queue_family_indices[] = {self->graphics_family, self->present_family};
        if (self->graphics_family != self->present_family) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queue_family_indices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = NULL;
        }

        if (vkCreateSwapchainKHR(self->device, &createInfo, MY_VK_ALLOCATOR, &(self->swap_chain)) != VK_SUCCESS) {
            LOG("Swap chain create failed!\n");
            break;
        }

        // retrieving images
        vkGetSwapchainImagesKHR(self->device, self->swap_chain, &(self->swap_chain_image_count), NULL);
        self->swap_chain_images = malloc(self->swap_chain_image_count * sizeof(VkImage));
        vkGetSwapchainImagesKHR(self->device, self->swap_chain, &(self->swap_chain_image_count), self->swap_chain_images);
        self->swap_chain_image_format = surface_format.format;
        self->swap_chain_extent = extent;

        ret = true;

    } while (false);

    free(details.formats);
    free(details.present_modes);
    
    return ret;

}

static VkImageView create_image_view_2d(my_application *self, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_levels) {
    VkImageViewCreateInfo view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = NULL,
        //VkImageViewCreateFlags     flags;
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY
        },
        .subresourceRange = {
            .aspectMask = aspect_flags,
            .baseMipLevel = 0,
            .levelCount = mip_levels,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    VkImageView image_view;
    if (VK_SUCCESS != vkCreateImageView(self->device, &view_info, MY_VK_ALLOCATOR, &image_view)) {
        LOG("Image view create failed!\n");
        return VK_NULL_HANDLE;
    }

    return image_view;
}

static bool create_swap_chain_image_views(my_application *self) {
    self->swap_chain_image_views = malloc(self->swap_chain_image_count * sizeof(VkImageView));

    for (uint32_t i = 0; i < self->swap_chain_image_count; ++i) {
        self->swap_chain_image_views[i] = create_image_view_2d(self, self->swap_chain_images[i], self->swap_chain_image_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        if (VK_NULL_HANDLE == self->swap_chain_image_views[i]) {
            LOG("Swap chain image view create failed with image index %d!\n", i);
            return false;
        }
    }

    return true;
}

static bool create_render_pass(my_application *self) {
    // attachments
    VkAttachmentDescription color_attachment = {
        //VkAttachmentDescriptionFlags    flags;
        .format = self->swap_chain_image_format,
        .samples = self->msaa_samplers,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentDescription depth_attachment = {
        //VkAttachmentDescriptionFlags    flags;
        .format = find_depth_format(self),
        .samples = self->msaa_samplers,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkAttachmentDescription color_attachment_resolve = {
        //VkAttachmentDescriptionFlags    flags;
        .format = self->swap_chain_image_format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    // subpass
    VkAttachmentReference color_attachment_ref = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference depth_attachment_ref = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference color_attachment_resolve_ref = {
        .attachment = 2,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass_desc = {
        //VkSubpassDescriptionFlags       flags;
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        //uint32_t                        inputAttachmentCount;
        //const VkAttachmentReference*    pInputAttachments;
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_ref,
        .pResolveAttachments = &color_attachment_resolve_ref,
        .pDepthStencilAttachment = &depth_attachment_ref,
        //uint32_t                        preserveAttachmentCount;
        //const uint32_t*                 pPreserveAttachments;
    };

    // render pass
    VkSubpassDependency subpass_dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
        //VkDependencyFlags       dependencyFlags;
    };
    VkAttachmentDescription attachments[3] = {color_attachment, depth_attachment, color_attachment_resolve};
    VkRenderPassCreateInfo render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = NULL,
        //VkRenderPassCreateFlags           flags;
        .attachmentCount = 3,
        .pAttachments = attachments,
        .subpassCount = 1,
        .pSubpasses = &subpass_desc,
        .dependencyCount = 1,
        .pDependencies = &subpass_dependency
    };

    if (VK_SUCCESS != vkCreateRenderPass(self->device, &render_pass_info, MY_VK_ALLOCATOR, &(self->render_pass))) {
        LOG("Render pass create failed!\n");
        return false;
    }

    return true;
}

static bool create_graphics_pipeline(my_application *self) {
    bool ret = true;

    // shader
    void *vert_shader_code = NULL;
    uint32_t vert_shader_length;
    VkShaderModule vert_shader_module;

    void *frag_shader_code = NULL;
    uint32_t frag_shader_length;
    VkShaderModule frag_shader_module;

    read_file(VERTEX_SHADER_PATH, &vert_shader_code, &vert_shader_length);
    vert_shader_module = create_shader_module(self, vert_shader_code, vert_shader_length);

    read_file(FRAGMENT_SHADER_PATH, &frag_shader_code, &frag_shader_length);
    frag_shader_module = create_shader_module(self, frag_shader_code, frag_shader_length);

    if (!vert_shader_module || !frag_shader_module) {
        LOG("Shader module create failed!\n");
        ret = false;
    }

    VkPipelineShaderStageCreateInfo shader_stage_info[2] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = NULL,
            //VkPipelineShaderStageCreateFlags    flags;
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vert_shader_module,
            .pName = "main",
            .pSpecializationInfo = NULL
        }, {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = NULL,
            //VkPipelineShaderStageCreateFlags    flags;
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = frag_shader_module,
            .pName = "main",
            .pSpecializationInfo = NULL
        }
    };

    // vertex input
    VkVertexInputBindingDescription vertex_binding_desc = {
        .binding = 0,
        .stride = sizeof(vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
    VkVertexInputAttributeDescription vertex_attr_descs[3] = {
        {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(vertex, position)
        }, {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(vertex, texcoord)
        }, {
            .location = 2,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(vertex, color)
        }
    };
    VkPipelineVertexInputStateCreateInfo vertex_input_state_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = NULL,
        //VkPipelineVertexInputStateCreateFlags       flags;
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertex_binding_desc,
        .vertexAttributeDescriptionCount = 3,
        .pVertexAttributeDescriptions = vertex_attr_descs
    };

    // input assembly
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = NULL,
        //VkPipelineInputAssemblyStateCreateFlags    flags;
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    // viewport and scissors
    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)self->swap_chain_extent.width,
        .height = (float)self->swap_chain_extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = self->swap_chain_extent
    };
    VkPipelineViewportStateCreateInfo viewport_state_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = NULL,
        //VkPipelineViewportStateCreateFlags    flags;
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };

    // rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer_state_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = NULL,
        //VkPipelineRasterizationStateCreateFlags    flags;
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f
    };

    // multisampling
    VkPipelineMultisampleStateCreateInfo multisampling_state_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = NULL,
        //VkPipelineMultisampleStateCreateFlags    flags;
        .rasterizationSamples = self->msaa_samplers,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = NULL,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE
    };

    //multisampling_state_info.sampleShadingEnable = VK_TRUE;
    //multisampling_state_info.minSampleShading = 0.2f;

    // depth and stencil testing
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = NULL,
        //VkPipelineDepthStencilStateCreateFlags    flags;
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        //VkStencilOpState                          front;
        //VkStencilOpState                          back;
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f
    };

    // color blending
    VkPipelineColorBlendAttachmentState color_blend_attachment_state = {
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };
    VkPipelineColorBlendStateCreateInfo color_blend_state_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = NULL,
        //VkPipelineColorBlendStateCreateFlags          flags;
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment_state,
        .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}
    };

    // dynamic state

    // pipeline layout
    VkDescriptorSetLayout set_layouts[] = {self->descriptor_set_layout};
    VkPipelineLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        //VkPipelineLayoutCreateFlags     flags;
        .setLayoutCount = 1,
        .pSetLayouts = set_layouts,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = NULL
    };
    if (VK_SUCCESS != vkCreatePipelineLayout(self->device, &layout_info, MY_VK_ALLOCATOR, &(self->pipeline_layout))) {
        LOG("Pipeline layout create failed!\n");
        ret = false;
    }

    // graphics pipeline
    VkGraphicsPipelineCreateInfo graphics_pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = NULL,
        //VkPipelineCreateFlags                            flags;
        .stageCount = 2,
        .pStages = shader_stage_info,
        .pVertexInputState = &vertex_input_state_info,
        .pInputAssemblyState = &input_assembly_state_info,
        //const VkPipelineTessellationStateCreateInfo*     pTessellationState;
        .pViewportState = &viewport_state_info,
        .pRasterizationState = &rasterizer_state_info,
        .pMultisampleState = &multisampling_state_info,
        .pDepthStencilState = &depth_stencil_state_info,
        .pColorBlendState = &color_blend_state_info,
        //const VkPipelineDynamicStateCreateInfo*          pDynamicState;
        .layout = self->pipeline_layout,
        .renderPass = self->render_pass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };
    if (VK_SUCCESS != vkCreateGraphicsPipelines(self->device, VK_NULL_HANDLE, 1, &graphics_pipeline_info, MY_VK_ALLOCATOR, &(self->pipeline))) {
        LOG("Pipeline create failed!\n");
        ret = false;
    }

    free(vert_shader_code);
    free(frag_shader_code);
    vkDestroyShaderModule(self->device, vert_shader_module, MY_VK_ALLOCATOR);
    vkDestroyShaderModule(self->device, frag_shader_module, MY_VK_ALLOCATOR);
    return ret;
}

static bool create_frame_buffers(my_application *self) {
    bool ret = true;
    self->swap_chain_frame_buffers = malloc(self->swap_chain_image_count * sizeof(VkFramebuffer));
    for (uint32_t i = 0; i < self->swap_chain_image_count; ++i) {
        VkImageView attachments[3] = {self->color_image_view, self->depth_image_view, self->swap_chain_image_views[i]};
        VkFramebufferCreateInfo frame_buffer_info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = NULL,
            //VkFramebufferCreateFlags    flags;
            .renderPass = self->render_pass,
            .attachmentCount = 3,
            .pAttachments = attachments,
            .width = self->swap_chain_extent.width,
            .height = self->swap_chain_extent.height,
            .layers = 1
        };

        if (VK_SUCCESS != vkCreateFramebuffer(self->device, &frame_buffer_info, MY_VK_ALLOCATOR, self->swap_chain_frame_buffers + i)) {
            LOG("Frame buffer %d create failed!\n", i);
            ret = false;
        }
    }
    return ret;
}

static VkShaderModule create_shader_module(my_application *self, void *shader_code, uint32_t length) {
    VkShaderModuleCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = NULL,
        //VkShaderModuleCreateFlags    flags;
        .codeSize = length,
        .pCode = shader_code
    };

    VkShaderModule shader_module;
    if (vkCreateShaderModule(self->device, &create_info, MY_VK_ALLOCATOR, &shader_module) != VK_SUCCESS) {
        return NULL;
    }

    return shader_module;
}

static bool create_command_pool(my_application *self) {
    VkCommandPoolCreateInfo command_pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = NULL,
        //VkCommandPoolCreateFlags    flags;
        .queueFamilyIndex = self->graphics_family
    };

    if (VK_SUCCESS != vkCreateCommandPool(self->device, &command_pool_info, MY_VK_ALLOCATOR, &(self->command_pool))) {
        LOG("Command pool create failed!\n");
        return false;
    }

    return true;
}

static bool create_command_buffers(my_application *self) {
    self->command_buffers = malloc(self->swap_chain_image_count * sizeof(VkCommandBuffer));
    VkCommandBufferAllocateInfo command_buffer_allocate = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = self->command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = self->swap_chain_image_count
    };
    if (VK_SUCCESS != vkAllocateCommandBuffers(self->device, &command_buffer_allocate, self->command_buffers)) {
        LOG("Command buffers alloc failed!\n");
        return false;
    }

    bool ret = true;
    for (uint32_t i = 0; i < self->swap_chain_image_count; ++i) {
        // begin commands
        VkCommandBufferBeginInfo cmd_begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = NULL,
            .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
            .pInheritanceInfo = NULL
        };
        if (VK_SUCCESS != vkBeginCommandBuffer(self->command_buffers[i], &cmd_begin_info)) {
            LOG("Begin command buffer %d failed!\n", i);
            ret = false;
            continue;
        }

        // begin render pass
        VkClearValue clear_value[2] = {
            {
                .color = {0.0f, 0.0f, 0.0f, 1.0f}
                //VkClearDepthStencilValue    depthStencil;
            }, {
                //VkClearColorValue           color;
                .depthStencil = {1.0f, 0}
            }
        };
        VkRenderPassBeginInfo render_pass_begin_info = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext = NULL,
            .renderPass = self->render_pass,
            .framebuffer = self->swap_chain_frame_buffers[i],
            .renderArea = {
                .offset = {.x = 0, .y = 0},
                .extent = self->swap_chain_extent
            },
            .clearValueCount = 2,
            .pClearValues = clear_value
        };
        vkCmdBeginRenderPass(self->command_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

        // draw commands
        vkCmdBindPipeline(self->command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, self->pipeline);

        VkBuffer vertex_buffers[] = {self->vertex_buffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(self->command_buffers[i], 0, 1, vertex_buffers, offsets);
        vkCmdBindIndexBuffer(self->command_buffers[i], self->index_buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(self->command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, self->pipeline_layout, 0, 1, self->descriptor_sets + i, 0, NULL);
        vkCmdDrawIndexed(self->command_buffers[i], self->index_count, 1, 0, 0, 0);

        // end render pass
        vkCmdEndRenderPass(self->command_buffers[i]);

        // end commands
        if (VK_SUCCESS != vkEndCommandBuffer(self->command_buffers[i])) {
            LOG("End command buffer %d failed!\n", i);
            ret = false;
        }
    }

    return ret;
}

static bool create_sync_objects(my_application *self) {
    bool ret = true;

    self->image_available_semaphores = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkSemaphore));
    self->render_finished_semaphores = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkSemaphore));
    self->flight_fences = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkFence));

    VkSemaphoreCreateInfo semaphore_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = NULL
        //VkSemaphoreCreateFlags    flags;
    };

    VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (VK_SUCCESS != vkCreateSemaphore(self->device, &semaphore_info, MY_VK_ALLOCATOR, self->image_available_semaphores + i)) {
            LOG("Image available semaphore %d create failed!\n", i);
            ret = false;
        }

        if (VK_SUCCESS != vkCreateSemaphore(self->device, &semaphore_info, MY_VK_ALLOCATOR, self->render_finished_semaphores + i)) {
            LOG("Render finished semaphore %d create failed!\n", i);
            ret = false;
        }

        if (VK_SUCCESS != vkCreateFence(self->device, &fence_info, MY_VK_ALLOCATOR, self->flight_fences + i)) {
            LOG("Flight fence %d create failed!\n", i);
            ret = false;
        }
    }

    return ret;
}

static int32_t find_memory_type(my_application *self, uint32_t type_filter, VkMemoryPropertyFlags property) {
    int32_t mem_type = -1;
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(self->physical_device, &mem_props);
    for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i) {
        if ((type_filter & (1 << i))
            && ((mem_props.memoryTypes[i].propertyFlags & property) == property)){
            mem_type = i;
            break;
        }
    }

    return mem_type;
}

static bool create_buffer(my_application *self, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags property, VkBuffer *buffer, VkDeviceMemory *buffer_mempry) {
    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        //VkBufferCreateFlags    flags;
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        //uint32_t               queueFamilyIndexCount;
        //const uint32_t*        pQueueFamilyIndices;
    };

    if (VK_SUCCESS != vkCreateBuffer(self->device, &buffer_info, MY_VK_ALLOCATOR, buffer)) {
        LOG("Buffer create failed!\n");
        return false;
    }

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(self->device, *buffer, &mem_reqs);

    int32_t mem_type = find_memory_type(self, mem_reqs.memoryTypeBits, property);
    if (mem_type < 0) {
        LOG("Find suitable memory type failed!n");
        return false;
    }

    VkMemoryAllocateInfo mem_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = NULL,
        .allocationSize = mem_reqs.size,
        .memoryTypeIndex = mem_type
    };
    if (VK_SUCCESS != vkAllocateMemory(self->device, &mem_alloc_info, MY_VK_ALLOCATOR, buffer_mempry)) {
        LOG("Allocate buffer memory failed!\n");
        return false;
    }

    vkBindBufferMemory(self->device, *buffer, *buffer_mempry, 0);

    return true;
}

static VkCommandBuffer begin_single_time_commands(my_application *self) {
    VkCommandBufferAllocateInfo command_buffer_allocate = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = self->command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    VkCommandBuffer command_buffer;
    if (VK_SUCCESS != vkAllocateCommandBuffers(self->device, &command_buffer_allocate, &command_buffer)) {
        LOG("Allocate command buffer failed!\n");
        return VK_NULL_HANDLE;
    }

    VkCommandBufferBeginInfo command_begin = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        //const VkCommandBufferInheritanceInfo*    pInheritanceInfo;
    };
    vkBeginCommandBuffer(command_buffer, &command_begin);

    return command_buffer;
}

static void end_single_time_commands(my_application *self, VkCommandBuffer command_buffer) {
    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = NULL,
        //uint32_t                       waitSemaphoreCount;
        //const VkSemaphore*             pWaitSemaphores;
        //const VkPipelineStageFlags*    pWaitDstStageMask;
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
        //uint32_t                       signalSemaphoreCount;
        //const VkSemaphore*             pSignalSemaphores;
    };
    vkQueueSubmit(self->graphics_queue, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(self->graphics_queue);

    vkFreeCommandBuffers(self->device, self->command_pool, 1, &command_buffer);
}

static bool copy_buffer(my_application *self, VkBuffer src, VkBuffer dst, VkDeviceSize size) {
    VkCommandBuffer command_buffer = begin_single_time_commands(self);
    if (VK_NULL_HANDLE == command_buffer) {
        return false;
    }

    VkBufferCopy buffer_region = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size
    };
    vkCmdCopyBuffer(command_buffer, src, dst, 1, &buffer_region);

    end_single_time_commands(self, command_buffer);

    return true;
}

static bool create_vertex_buffer(my_application *self) {
    VkDeviceSize buffer_size = self->vertex_count * sizeof(vertex);

    bool ret = true;
    VkBuffer staging_buffer = VK_NULL_HANDLE;
    VkDeviceMemory staging_buffer_memory = VK_NULL_HANDLE;

    do {
        if (false == create_buffer(self, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, &staging_buffer_memory)) {
            LOG("Staging buffer create failed!\n");
            ret = false;
            break;
        }

        void *data = NULL;
        if (VK_SUCCESS != vkMapMemory(self->device, staging_buffer_memory, 0, buffer_size, 0, &data)) {
            LOG("Map staging buffer memory failed!\n");
            ret = false;
            break;
        }

        memcpy(data, self->vertices, (size_t)buffer_size);
        vkUnmapMemory(self->device, staging_buffer_memory);

        if (false == create_buffer(self, buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &(self->vertex_buffer), &(self->vertex_buffer_memory))) {
            LOG("Vertex buffer create failed!\n");
            ret = false;
            break;
        }

        if (false == copy_buffer(self, staging_buffer, self->vertex_buffer, buffer_size)) {
            LOG("Copy to vertex buffer failed!\n");
            ret = false;
            break;
        }

    } while(false);

    if (staging_buffer) {
        vkDestroyBuffer(self->device, staging_buffer, MY_VK_ALLOCATOR);
    }

    if (staging_buffer_memory) {
        vkFreeMemory(self->device, staging_buffer_memory, MY_VK_ALLOCATOR);
    }

    return ret;
}

static bool create_index_buffer(my_application *self) {
    VkDeviceSize buffer_size = self->index_count * sizeof(uint32_t);

    bool ret = true;
    VkBuffer staging_buffer = VK_NULL_HANDLE;
    VkDeviceMemory staging_buffer_memory = VK_NULL_HANDLE;

    do {
        if (false == create_buffer(self, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, &staging_buffer_memory)) {
            LOG("Staging buffer create failed!\n");
            ret = false;
            break;
        }

        void *data = NULL;
        if (VK_SUCCESS != vkMapMemory(self->device, staging_buffer_memory, 0, buffer_size, 0, &data)) {
            LOG("Map staging buffer memory failed!\n");
            ret = false;
            break;
        }

        memcpy(data, self->indices, (size_t)buffer_size);
        vkUnmapMemory(self->device, staging_buffer_memory);

        if (false == create_buffer(self, buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &(self->index_buffer), &(self->index_buffer_memory))) {
            LOG("Index buffer create failed!\n");
            ret = false;
            break;
        }

        if (false == copy_buffer(self, staging_buffer, self->index_buffer, buffer_size)) {
            LOG("Copy to index buffer failed!\n");
            ret = false;
            break;
        }

    } while(false);

    if (staging_buffer) {
        vkDestroyBuffer(self->device, staging_buffer, MY_VK_ALLOCATOR);
    }

    if (staging_buffer_memory) {
        vkFreeMemory(self->device, staging_buffer_memory, MY_VK_ALLOCATOR);
    }

    return ret;
}

static bool create_descriptor_set_layout(my_application *self) {
    VkDescriptorSetLayoutBinding layout_bindings[2] = {
        {
            // ubo
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = NULL,
        }, {
            // sampler
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = NULL,
        }
    };

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        //VkDescriptorSetLayoutCreateFlags       flags;
        .bindingCount = 2,
        .pBindings = layout_bindings
    };

    if (VK_SUCCESS != vkCreateDescriptorSetLayout(self->device, &descriptor_set_layout_info, MY_VK_ALLOCATOR, &(self->descriptor_set_layout))) {
        LOG("Create descriptor set layout failed!\n");
        return false;
    }

    return true;
}

static bool create_uniform_buffers(my_application *self) {
    VkDeviceSize buffer_size = sizeof(uniform_buffer_object);
    self->uniform_buffers = malloc(self->swap_chain_image_count * sizeof(VkBuffer));
    self->uniform_buffer_memories = malloc(self->swap_chain_image_count * sizeof(VkDeviceMemory));

    bool ret = true;
    for (uint32_t i = 0; i < self->swap_chain_image_count; ++i) {
        if (false == create_buffer(self, buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, self->uniform_buffers + i, self->uniform_buffer_memories + i)) {
            LOG("Create uniform buffer %d failed!\n", i);
            ret = false;
        }
    }

    return ret;
}

static bool create_descriptor_pool(my_application *self) {
    VkDescriptorPoolSize pool_size[2] = {
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = self->swap_chain_image_count
        }, {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = self->swap_chain_image_count
        }
    };

    VkDescriptorPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = NULL,
        //VkDescriptorPoolCreateFlags    flags;
        .maxSets = self->swap_chain_image_count,
        .poolSizeCount = 2,
        .pPoolSizes = pool_size
    };

    if (VK_SUCCESS != vkCreateDescriptorPool(self->device, &pool_info, MY_VK_ALLOCATOR, &(self->descriptor_pool))) {
        LOG("Create descriptor pool failed!\n");
        return false;
    }

    return true;
}

static bool create_descriptor_set(my_application *self) {
    uint32_t layout_count = self->swap_chain_image_count;
    VkDescriptorSetLayout *layouts = malloc(layout_count * sizeof(VkDescriptorSetLayout));
    for (uint32_t i = 0; i < layout_count; ++i) {
        layouts[i] = self->descriptor_set_layout;
    }

    VkDescriptorSetAllocateInfo desc_set_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = NULL,
        .descriptorPool = self->descriptor_pool,
        .descriptorSetCount = layout_count,
        .pSetLayouts = layouts
    };

    bool ret = true;
    self->descriptor_sets = malloc(layout_count * sizeof(VkDescriptorSet));
    if (VK_SUCCESS != vkAllocateDescriptorSets(self->device, &desc_set_alloc_info, self->descriptor_sets)) {
        LOG("Allocate descriptor sets failed!\n");
        ret = false;
    } else {
        for (uint32_t i = 0; i < layout_count; ++i) {
            VkDescriptorBufferInfo buffer_info = {
                .buffer = self->uniform_buffers[i],
                .offset = 0,
                .range = sizeof(uniform_buffer_object)
            };

            VkDescriptorImageInfo image_info = {
                .sampler = self->texture_sampler,
                .imageView = self->texture_image_view,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };

            VkWriteDescriptorSet write_desc_set[2] = {
                {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext = NULL,
                    .dstSet = self->descriptor_sets[i],
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pImageInfo = NULL,
                    .pBufferInfo = &buffer_info,
                    .pTexelBufferView = NULL
                }, {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext = NULL,
                    .dstSet = self->descriptor_sets[i],
                    .dstBinding = 1,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = &image_info,
                    .pBufferInfo = NULL,
                    .pTexelBufferView = NULL
                }
            };
            vkUpdateDescriptorSets(self->device, 2, write_desc_set, 0, NULL);
        }
    }

    free(layouts);
    return ret;
}

static bool create_image_2d(my_application *self, uint32_t width, uint32_t height, uint32_t mip_levels, VkSampleCountFlagBits samplers, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage *image, VkDeviceMemory *image_memory) {
    VkImageCreateInfo image_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        //VkImageCreateFlags       flags;
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = {
            .width = width,
            .height = height,
            .depth = 1
        },
        .mipLevels = mip_levels,
        .arrayLayers = 1,
        .samples = samplers,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        //uint32_t                 queueFamilyIndexCount;
        //const uint32_t*          pQueueFamilyIndices;
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    if (VK_SUCCESS != vkCreateImage(self->device, &image_info, MY_VK_ALLOCATOR, image)) {
        LOG("Texture image create failed!\n");
        return false;
    }

    VkMemoryRequirements mem_reqs;
    vkGetImageMemoryRequirements(self->device, *image, &mem_reqs);

    int32_t mem_type = find_memory_type(self, mem_reqs.memoryTypeBits, properties);
    if (mem_type < 0) {
        LOG("Find suitable memory type failed!\n");
        return false;
    }

    VkMemoryAllocateInfo mem_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = NULL,
        .allocationSize = mem_reqs.size,
        .memoryTypeIndex = mem_type
    };
    if (VK_SUCCESS != vkAllocateMemory(self->device, &mem_alloc_info, MY_VK_ALLOCATOR, image_memory)) {
        LOG("Allocate texture memory failed!\n");
        return false;
    }

    vkBindImageMemory(self->device, *image, *image_memory, 0);

    return true;
}

static void transition_image_layout(my_application *self, VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout, uint32_t mip_levels) {
    VkCommandBuffer command_buffer = begin_single_time_commands(self);
    if (VK_NULL_HANDLE == command_buffer) {
        return;
    }

    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = NULL,
        .srcAccessMask = 0,
        .dstAccessMask = 0,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = mip_levels,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (has_stencil_component(format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }

    VkPipelineStageFlags source_stage;
    VkPipelineStageFlags dest_stage;
    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED  && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dest_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dest_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED  && new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dest_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED  && new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dest_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }

    vkCmdPipelineBarrier(command_buffer, source_stage, dest_stage, 0, 0, NULL, 0, NULL, 1, &barrier);

    end_single_time_commands(self, command_buffer);
}

static bool generate_mipmaps(my_application *self, VkImage image, VkFormat format, int32_t width, int32_t height, uint32_t mip_levels) {
    //VkFormatProperties props;
    //vkGetPhysicalDeviceFormatProperties(self->physical_device, format, &props);
    //if (!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
    //    LOG("Build-in blit image not support! Mipmaps should be generated manually!\n"); 
    //    return false;
    //}
    if (VK_FORMAT_UNDEFINED == find_supported_format(self, &format, 1, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        LOG("Build-in blit image not support! Mipmaps should be generated manually!\n"); 
        return false;
    }

    VkCommandBuffer command_buffer = begin_single_time_commands(self);
    if (VK_NULL_HANDLE == command_buffer) {
        return false;
    }

    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = NULL,
        //.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        //.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        //.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        //.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            //.baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    int32_t mip_width = width;
    int32_t mip_height = height;
    for (uint32_t i = 1; i < mip_levels; ++i) {
        int32_t nl_width = mip_width > 1 ? mip_width / 2 : 1;
        int32_t nl_height = mip_height > 1 ? mip_height / 2 : 1;

        barrier.subresourceRange.baseMipLevel = i - 1;

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

        VkImageBlit blit = {
            .srcSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = i - 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .srcOffsets = { {0, 0, 0}, {mip_width, mip_height, 1} },
            .dstSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = i,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .dstOffsets = { {0, 0, 0}, {nl_width, nl_height, 1} }
        };

        vkCmdBlitImage(command_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

        mip_width = nl_width;
        mip_height = nl_height;
    }

    // translate last mipmap level
    barrier.subresourceRange.baseMipLevel = mip_levels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

    end_single_time_commands(self, command_buffer);

    return true;
}

static void copy_buffer_to_image(my_application *self, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer command_buffer = begin_single_time_commands(self);
    if (VK_NULL_HANDLE == command_buffer) {
        return;
    }

    VkBufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .imageOffset = {
            .x = 0,
            .y = 0,
            .z = 0
        },
        .imageExtent = {
            .width = width,
            .height = height,
            .depth = 1    
        }
    };
    vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    end_single_time_commands(self, command_buffer);
}

static bool create_texture_image(my_application *self) {
    int width, height, channels;
    stbi_uc *pixels = stbi_load(TEXTURE_PATH, &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels) {
        LOG("Load image file failed!\n");
        return false;
    }

    float length = (float)(width > height ? width : height);
    uint32_t mip_levels = (uint32_t)floorf(log2f(length)) + 1;
    self->mip_levels = mip_levels;

    VkDeviceSize buffer_size = width * height * 4;
    VkBuffer staging_buffer = VK_NULL_HANDLE;
    VkDeviceMemory staging_buffer_memory = VK_NULL_HANDLE;

    bool ret = true;
    do {
        if (false == create_buffer(self, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, &staging_buffer_memory)) {
            LOG("Staging buffer create failed!\n");
            ret = false;
            break;
        }

        void *data = NULL;
        if (VK_SUCCESS != vkMapMemory(self->device, staging_buffer_memory, 0, buffer_size, 0, &data)) {
            LOG("Map staging buffer memory failed!\n");
            ret = false;
            break;
        }

        memcpy(data, pixels, (size_t)buffer_size);
        vkUnmapMemory(self->device, staging_buffer_memory);

        if (false == create_image_2d(self, (uint32_t)width, (uint32_t)height, mip_levels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &(self->texture_image), &(self->texture_image_memory))) {
            LOG("Create a 2d image failed!\n");
            ret = false;
            break;
        }

        transition_image_layout(self, self->texture_image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mip_levels);
        copy_buffer_to_image(self, staging_buffer, self->texture_image, (uint32_t)width, (uint32_t)height);
        //transition_image_layout(self, self->texture_image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mip_levels);
        if (generate_mipmaps(self, self->texture_image, VK_FORMAT_R8G8B8A8_UNORM, width, height, mip_levels)) {
            // generate mipmaps here 
        }

        vkDestroyBuffer(self->device, staging_buffer, MY_VK_ALLOCATOR);
        vkFreeMemory(self->device, staging_buffer_memory, MY_VK_ALLOCATOR);
    } while(false);

    stbi_image_free(pixels);

    return ret;
}

static bool create_texture_image_view(my_application *self) {
    self->texture_image_view = create_image_view_2d(self, self->texture_image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, self->mip_levels);
    if (VK_NULL_HANDLE == self->texture_image_view) {
        LOG("Texture image view create failed!\n");
        return false;
    }

    return true;
}

static bool create_texture_sampler(my_application *self) {
    VkSamplerCreateInfo sampler_info = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = NULL,
        //VkSamplerCreateFlags    flags;
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = 16,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.0f,
        .maxLod = self->mip_levels - 1.0f,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE
    };

    if (VK_SUCCESS != vkCreateSampler(self->device, &sampler_info, MY_VK_ALLOCATOR, &(self->texture_sampler))) {
        LOG("Create texture sampler failed!\n");
        return false;
    }

    return true;
}

static VkFormat find_supported_format(my_application *self, VkFormat *formats, uint32_t count, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (uint32_t i = 0; i < count; ++i) {
        VkFormat format = formats[i];
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(self->physical_device, format, &props);
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    LOG("Suitable format not found!\n");
    return VK_FORMAT_UNDEFINED;
}

static bool has_stencil_component(VkFormat format) {
    return (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT);
}

static VkFormat find_depth_format(my_application *self) {
    VkFormat depth_formats[3] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    return find_supported_format(self, depth_formats, 3, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

static bool create_depth_resources(my_application *self) {
    VkFormat format = find_depth_format(self);
    if (format == VK_FORMAT_UNDEFINED) {
        return false;
    }

    uint32_t count = self->swap_chain_image_count;
    uint32_t width = self->swap_chain_extent.width;
    uint32_t height = self->swap_chain_extent.height;

    if (false == create_image_2d(self, width, height, 1, self->msaa_samplers, format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &(self->depth_image), &(self->depth_image_memory))) {
        LOG("Create depth image failed!\n");
        return false;
    }

    VkImageView image_view = create_image_view_2d(self, self->depth_image, format, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
    if (VK_NULL_HANDLE == image_view) {
        LOG("Create depth image view failed!\n");
        return false;
    }

    transition_image_layout(self, self->depth_image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);

    self->depth_image_view = image_view;
    self->depth_format = format;

    return true;
}

static bool load_model_source(my_application *self) {
    char *obj_content = NULL;
    uint32_t obj_size = 0;

    if (!read_file(MODEL_SRC_PATH, &obj_content, &obj_size)) {
        LOG("Read model file failed!\n");
        return false;
    }

    tinyobj_attrib_t obj_attrib;
    tinyobj_shape_t *obj_shapes = NULL;
    size_t shape_count = 0;
    tinyobj_material_t *obj_materials = NULL;
    size_t material_count = 0;
    if (TINYOBJ_SUCCESS == tinyobj_parse_obj(&obj_attrib, &obj_shapes, &shape_count, &obj_materials, &material_count, obj_content, obj_size, TINYOBJ_FLAG_TRIANGULATE)) {
        uint32_t face_count = 0;
        for (size_t i = 0; i < shape_count; ++i) {
            face_count += obj_shapes[i].length;
        }
        uint32_t vertex_count = 3 * face_count;
        vertex *vertices = malloc(vertex_count * sizeof(vertex));
        uint32_t index_count = vertex_count;
        uint32_t *indices = malloc(index_count * sizeof(uint32_t));
        uint32_t next_vertex_index = 0;
        for (size_t i = 0; i < shape_count; ++i) {
            for (uint32_t j = obj_shapes[i].face_offset; j < obj_shapes[i].face_offset + obj_shapes[i].length; ++j) {
                for (uint32_t m = 0; m < 3; ++m) {
                    uint32_t index = 3 * j + m;
                    float *position = obj_attrib.vertices + obj_attrib.faces[index].v_idx * 3;
                    float *texcoord = obj_attrib.texcoords + obj_attrib.faces[index].vt_idx * 2;
                    vertex v = {
                        {position[0], position[1], position[2]},
                        {texcoord[0], 1.0f - texcoord[1]},
                        {1.0f, 1.0f, 1.0f}
                    };
                    uint32_t vertex_index = next_vertex_index;
                    for (uint32_t n = 0; n < next_vertex_index; ++n) {
                        if (vertices[n].position[0] == v.position[0]
                            && vertices[n].position[1] == v.position[1]
                            && vertices[n].position[2] == v.position[2]
                            && vertices[n].texcoord[0] == v.texcoord[0]
                            && vertices[n].texcoord[1] == v.texcoord[1]) {
                            vertex_index = n;
                            break;
                        }
                    }
                    if (vertex_index == next_vertex_index) {
                        vertex *vert = vertices + vertex_index;
                        *vert = v;
                        ++ next_vertex_index;
                    }
                    indices[index] = vertex_index;
                }
            }
        }

        uint32_t final_vertex_count = next_vertex_index;
        self->vertex_count = final_vertex_count;
        self->vertices = malloc(final_vertex_count * sizeof(vertex));
        memcpy(self->vertices, vertices, final_vertex_count * sizeof(vertex));
        free(vertices);

        self->indices = indices;
        self->index_count = index_count;

        // write to file
        FILE *binFile = fopen(MODEL_BIN_PATH, "wb");
        if (binFile) {
            fwrite(&(self->vertex_count), sizeof(uint32_t), 1, binFile);
            fwrite(&(self->index_count), sizeof(uint32_t), 1, binFile);
            fwrite(self->vertices, sizeof(vertex), (size_t)self->vertex_count, binFile);
            fwrite(self->indices, sizeof(uint32_t), (size_t)self->index_count, binFile);
            fclose(binFile);
        }

        tinyobj_attrib_free(&obj_attrib);
        tinyobj_shapes_free(obj_shapes, shape_count);
        tinyobj_materials_free(obj_materials, material_count);

        return true;
    }

    LOG("Load model file failed!\n");
    free(obj_content);
    return false;
}

static bool load_model_binary(my_application *self) {
    char *content = NULL;
    uint32_t size = 0;

    if (!read_file(MODEL_BIN_PATH, &content, &size)) {
        LOG("Read model binary file failed!\n");
        return false;
    }

    char *buffer = content;
    size_t count_size = sizeof(uint32_t);

    self->vertex_count = *((uint32_t *)buffer);
    buffer += count_size;

    self->index_count = *((uint32_t *)buffer);
    buffer += count_size;

    size_t vertex_buffer_size = self->vertex_count * sizeof(vertex);
    self->vertices = malloc(vertex_buffer_size);
    memcpy(self->vertices, buffer, vertex_buffer_size);
    buffer += vertex_buffer_size;

    size_t index_buffer_size = self->index_count * sizeof(uint32_t);
    self->indices = malloc(index_buffer_size);
    memcpy(self->indices, buffer, index_buffer_size);
    buffer += index_buffer_size;

    free(content);
    return true;
}

static VkSampleCountFlagBits get_max_usable_sample_count(my_application *self) {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(self->physical_device, &props);

    VkSampleCountFlags counts = MIN(props.limits.framebufferColorSampleCounts, props.limits.framebufferDepthSampleCounts);
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}

static bool create_color_resources(my_application *self) {
    VkFormat color_format = self->swap_chain_image_format;
    uint32_t width = self->swap_chain_extent.width;
    uint32_t height = self->swap_chain_extent.height;

    if (false == create_image_2d(self, width, height, 1, self->msaa_samplers, color_format, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &(self->color_image), &(self->color_image_memory))) {
        LOG("Create msaa image failed!\n");
        return false;
    }

    VkImageView image_view = create_image_view_2d(self, self->color_image, color_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    if (VK_NULL_HANDLE == image_view) {
        LOG("Create msaa image view failed!\n");
        return false;
    }

    self->color_image_view = image_view;
    transition_image_layout(self, self->color_image, color_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);

    return true;
}

static void cleanup_swap_chain(my_application *self) {
    if (self->color_image_view) {
        vkDestroyImageView(self->device, self->color_image_view, MY_VK_ALLOCATOR);
    }

    if (self->color_image) {
        vkDestroyImage(self->device, self->color_image, MY_VK_ALLOCATOR);
    }

    if (self->color_image_memory) {
        vkFreeMemory(self->device, self->color_image_memory, MY_VK_ALLOCATOR);
    }

    if (self->depth_image_view) {
        vkDestroyImageView(self->device, self->depth_image_view, MY_VK_ALLOCATOR);
    }

    if (self->depth_image) {
        vkDestroyImage(self->device, self->depth_image, MY_VK_ALLOCATOR);
    }

    if (self->depth_image_memory) {
        vkFreeMemory(self->device, self->depth_image_memory, MY_VK_ALLOCATOR);
    }

    if (self->swap_chain_frame_buffers) {
        for (uint32_t i = 0; i < self->swap_chain_image_count; ++i) {
            vkDestroyFramebuffer(self->device, self->swap_chain_frame_buffers[i], MY_VK_ALLOCATOR);
        }
        free(self->swap_chain_frame_buffers);
    }

    if (self->command_buffers) {
        vkFreeCommandBuffers(self->device, self->command_pool, self->swap_chain_image_count, self->command_buffers);
        free(self->command_buffers);
    }

    if (self->pipeline) {
        vkDestroyPipeline(self->device, self->pipeline, MY_VK_ALLOCATOR);
    }

    if (self->pipeline_layout) {
        vkDestroyPipelineLayout(self->device, self->pipeline_layout, MY_VK_ALLOCATOR);
    }

    if (self->render_pass) {
        vkDestroyRenderPass(self->device, self->render_pass, MY_VK_ALLOCATOR);
    }

    if (self->swap_chain_image_views) {
        for (uint32_t i = 0; i < self->swap_chain_image_count; ++i) {
            vkDestroyImageView(self->device, self->swap_chain_image_views[i], MY_VK_ALLOCATOR);
        }
        free(self->swap_chain_image_views);
    }

    if (self->swap_chain_images) {
        free(self->swap_chain_images);
    }

    if (self->swap_chain) {
        vkDestroySwapchainKHR(self->device, self->swap_chain, MY_VK_ALLOCATOR);
    }
}

static void recreate_swap_chain(my_application *self) {
    // wait util window become front if minimize it before
    int width = 0, height = 0;
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(self->window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(self->device);

    cleanup_swap_chain(self);

    create_swap_chain(self);
    create_swap_chain_image_views(self);
    create_render_pass(self);
    create_graphics_pipeline(self);
    create_color_resources(self);
    create_depth_resources(self);
    create_frame_buffers(self);
    create_command_buffers(self);
}

static void draw_frame(my_application *self) {
    vkWaitForFences(self->device, 1, &(self->flight_fences[self->current_frame]), VK_TRUE, UINT64_MAX);
    vkResetFences(self->device, 1, &(self->flight_fences[self->current_frame]));

    uint32_t image_index;
    VkResult ret = vkAcquireNextImageKHR(self->device, self->swap_chain, UINT64_MAX, self->image_available_semaphores[self->current_frame], VK_NULL_HANDLE, &image_index);
    if (VK_ERROR_OUT_OF_DATE_KHR == ret) {
        recreate_swap_chain(self);
        return;
    } else if (VK_SUCCESS != ret && VK_SUBOPTIMAL_KHR != ret) {
        LOG("Acquire swap chain image failed!\n");
        return;
    }

    update_uniform_buffer(self, image_index);

    VkSemaphore wait_semaphores[] = {self->image_available_semaphores[self->current_frame]};
    VkPipelineStageFlags wait_stage_flags[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signal_semaphores[] = {self->render_finished_semaphores[self->current_frame]};

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = NULL,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = wait_semaphores,
        .pWaitDstStageMask = wait_stage_flags,
        .commandBufferCount = 1,
        .pCommandBuffers = self->command_buffers + image_index,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signal_semaphores
    };

    if (VK_SUCCESS != vkQueueSubmit(self->graphics_queue, 1, &submit_info, self->flight_fences[self->current_frame])) {
        LOG("Graphics queue submit failed!\n");
        return;
    }

    VkSwapchainKHR swap_chains[] = {self->swap_chain};
    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = NULL,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signal_semaphores,
        .swapchainCount = 1,
        .pSwapchains = swap_chains,
        .pImageIndices = &image_index,
        .pResults = NULL
    };
    ret = vkQueuePresentKHR(self->present_queue, &present_info);
    if (VK_ERROR_OUT_OF_DATE_KHR == ret || VK_SUBOPTIMAL_KHR == ret || self->frame_buffer_resized) {
        self->frame_buffer_resized = false;
        recreate_swap_chain(self);
    } else if (VK_SUCCESS != ret) {
        LOG("Present swap chain image failed!\n");
    }

    self->current_frame = (self->current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

static void update_uniform_buffer(my_application *self, uint32_t index) {
    float time = high_resolution_clock_now();
    uniform_buffer_object ubo;
    glm_mat4_identity(ubo.model);
    glm_rotate(ubo.model, time * glm_rad(30.0f), (vec3){0.0f, 0.0f, 1.0f});
    glm_lookat((vec3){2.0f, 2.0f, 2.0f}, (vec3){0.0f, 0.0f, 0.0f}, (vec3){0.0f, 0.0f, 1.0f}, ubo.view);
    float aspect = (float)self->swap_chain_extent.width / (float) self->swap_chain_extent.height;
    glm_perspective_zto(glm_rad(45.0f), aspect, 0.1f, 10.0f, ubo.proj);
    ubo.proj[1][1] *= -1;

    size_t buffer_size = sizeof(uniform_buffer_object);
    void *data = NULL;
    vkMapMemory(self->device, self->uniform_buffer_memories[index], 0, buffer_size, 0, &data);
    memcpy(data, &ubo, buffer_size);
    vkUnmapMemory(self->device, self->uniform_buffer_memories[index]);
}

static bool read_file(const char *file_name, void **content, uint32_t *length) {
    FILE *file = fopen(file_name, "rb");
    if (!file) {
        return false;
    }

    fseek(file, 0, SEEK_END);
    long len = ftell(file);
    fseek(file, 0, SEEK_SET);

    void *con = malloc(len * sizeof(char));
    fread(con, sizeof(char), len, file);
    fclose(file);

    *content = con;
    *length = len;
    return true;
}

