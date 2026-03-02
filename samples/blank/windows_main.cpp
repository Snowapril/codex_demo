#define WIN32_LEAN_AND_MEAN
#include <vulkan/vulkan.h>
#include <windows.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {
const uint32_t kWidth = 800;
const uint32_t kHeight = 600;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  switch (msg) {
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
    default:
      return DefWindowProc(hwnd, msg, wparam, lparam);
  }
}

struct VulkanState {
  VkInstance instance = VK_NULL_HANDLE;
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;
  VkQueue graphicsQueue = VK_NULL_HANDLE;
  uint32_t graphicsQueueFamily = 0;
  VkSwapchainKHR swapchain = VK_NULL_HANDLE;
  VkFormat swapchainFormat = VK_FORMAT_B8G8R8A8_UNORM;
  VkExtent2D swapchainExtent = {};
  std::vector<VkImage> images;
  std::vector<VkImageView> imageViews;
  VkRenderPass renderPass = VK_NULL_HANDLE;
  std::vector<VkFramebuffer> framebuffers;
  VkCommandPool commandPool = VK_NULL_HANDLE;
  std::vector<VkCommandBuffer> commandBuffers;
  VkSemaphore imageAvailable = VK_NULL_HANDLE;
  VkSemaphore renderFinished = VK_NULL_HANDLE;
  VkFence inFlight = VK_NULL_HANDLE;
};

bool check(VkResult result, const char* msg) {
  if (result != VK_SUCCESS) {
    std::cerr << msg << " (VkResult=" << result << ")\n";
    return false;
  }
  return true;
}

bool createInstance(VulkanState& state) {
  VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
  appInfo.pApplicationName = "Blank Vulkan Sample";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "reng";
  appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
  appInfo.apiVersion = VK_API_VERSION_1_2;

  std::array<const char*, 2> extensions = {VK_KHR_SURFACE_EXTENSION_NAME,
                                           VK_KHR_WIN32_SURFACE_EXTENSION_NAME};

  VkInstanceCreateInfo createInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  return check(vkCreateInstance(&createInfo, nullptr, &state.instance),
               "vkCreateInstance failed");
}

bool createSurface(VulkanState& state, HINSTANCE hInstance, HWND hwnd) {
  VkWin32SurfaceCreateInfoKHR createInfo{
      VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
  createInfo.hinstance = hInstance;
  createInfo.hwnd = hwnd;
  return check(vkCreateWin32SurfaceKHR(state.instance, &createInfo, nullptr,
                                       &state.surface),
               "vkCreateWin32SurfaceKHR failed");
}

bool pickPhysicalDevice(VulkanState& state) {
  uint32_t count = 0;
  vkEnumeratePhysicalDevices(state.instance, &count, nullptr);
  if (count == 0) {
    std::cerr << "No Vulkan physical devices found\n";
    return false;
  }
  std::vector<VkPhysicalDevice> devices(count);
  vkEnumeratePhysicalDevices(state.instance, &count, devices.data());
  state.physicalDevice = devices[0];
  return true;
}

bool findQueueFamily(VulkanState& state) {
  uint32_t count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(state.physicalDevice, &count,
                                           nullptr);
  std::vector<VkQueueFamilyProperties> props(count);
  vkGetPhysicalDeviceQueueFamilyProperties(state.physicalDevice, &count,
                                           props.data());

  for (uint32_t i = 0; i < count; ++i) {
    VkBool32 presentSupport = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(state.physicalDevice, i, state.surface,
                                         &presentSupport);
    if ((props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentSupport) {
      state.graphicsQueueFamily = i;
      return true;
    }
  }
  return false;
}

bool createDevice(VulkanState& state) {
  float priority = 1.0f;
  VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
  queueInfo.queueFamilyIndex = state.graphicsQueueFamily;
  queueInfo.queueCount = 1;
  queueInfo.pQueuePriorities = &priority;

  const char* extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  VkDeviceCreateInfo createInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  createInfo.queueCreateInfoCount = 1;
  createInfo.pQueueCreateInfos = &queueInfo;
  createInfo.enabledExtensionCount = 1;
  createInfo.ppEnabledExtensionNames = extensions;

  if (!check(vkCreateDevice(state.physicalDevice, &createInfo, nullptr,
                            &state.device),
             "vkCreateDevice failed")) {
    return false;
  }

  vkGetDeviceQueue(state.device, state.graphicsQueueFamily, 0,
                   &state.graphicsQueue);
  return true;
}

bool createSwapchain(VulkanState& state) {
  VkSurfaceCapabilitiesKHR caps{};
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(state.physicalDevice, state.surface,
                                            &caps);

  uint32_t formatCount = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(state.physicalDevice, state.surface,
                                       &formatCount, nullptr);
  std::vector<VkSurfaceFormatKHR> formats(formatCount);
  vkGetPhysicalDeviceSurfaceFormatsKHR(state.physicalDevice, state.surface,
                                       &formatCount, formats.data());

  VkSurfaceFormatKHR chosenFormat = formats[0];
  for (const auto& fmt : formats) {
    if (fmt.format == VK_FORMAT_B8G8R8A8_UNORM &&
        fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      chosenFormat = fmt;
      break;
    }
  }

  state.swapchainFormat = chosenFormat.format;
  state.swapchainExtent = caps.currentExtent.width != UINT32_MAX
                              ? caps.currentExtent
                              : VkExtent2D{kWidth, kHeight};

  uint32_t imageCount = caps.minImageCount + 1;
  if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount) {
    imageCount = caps.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo{
      VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
  createInfo.surface = state.surface;
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = state.swapchainFormat;
  createInfo.imageColorSpace = chosenFormat.colorSpace;
  createInfo.imageExtent = state.swapchainExtent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  createInfo.preTransform = caps.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
  createInfo.clipped = VK_TRUE;

  if (!check(vkCreateSwapchainKHR(state.device, &createInfo, nullptr,
                                  &state.swapchain),
             "vkCreateSwapchainKHR failed")) {
    return false;
  }

  vkGetSwapchainImagesKHR(state.device, state.swapchain, &imageCount, nullptr);
  state.images.resize(imageCount);
  vkGetSwapchainImagesKHR(state.device, state.swapchain, &imageCount,
                          state.images.data());

  state.imageViews.resize(state.images.size());
  for (size_t i = 0; i < state.images.size(); ++i) {
    VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewInfo.image = state.images[i];
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = state.swapchainFormat;
    viewInfo.components = {
        VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (!check(vkCreateImageView(state.device, &viewInfo, nullptr,
                                 &state.imageViews[i]),
               "vkCreateImageView failed")) {
      return false;
    }
  }

  return true;
}

bool createRenderPass(VulkanState& state) {
  VkAttachmentDescription colorAttachment{};
  colorAttachment.format = state.swapchainFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorRef{};
  colorRef.attachment = 0;
  colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorRef;

  VkRenderPassCreateInfo renderPassInfo{
      VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;

  return check(vkCreateRenderPass(state.device, &renderPassInfo, nullptr,
                                  &state.renderPass),
               "vkCreateRenderPass failed");
}

bool createFramebuffers(VulkanState& state) {
  state.framebuffers.resize(state.imageViews.size());
  for (size_t i = 0; i < state.imageViews.size(); ++i) {
    VkImageView attachments[] = {state.imageViews[i]};
    VkFramebufferCreateInfo fbInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    fbInfo.renderPass = state.renderPass;
    fbInfo.attachmentCount = 1;
    fbInfo.pAttachments = attachments;
    fbInfo.width = state.swapchainExtent.width;
    fbInfo.height = state.swapchainExtent.height;
    fbInfo.layers = 1;

    if (!check(vkCreateFramebuffer(state.device, &fbInfo, nullptr,
                                   &state.framebuffers[i]),
               "vkCreateFramebuffer failed")) {
      return false;
    }
  }
  return true;
}

bool createCommandPool(VulkanState& state) {
  VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  poolInfo.queueFamilyIndex = state.graphicsQueueFamily;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  return check(
      vkCreateCommandPool(state.device, &poolInfo, nullptr, &state.commandPool),
      "vkCreateCommandPool failed");
}

bool createCommandBuffers(VulkanState& state) {
  state.commandBuffers.resize(state.framebuffers.size());
  VkCommandBufferAllocateInfo allocInfo{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  allocInfo.commandPool = state.commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount =
      static_cast<uint32_t>(state.commandBuffers.size());

  return check(vkAllocateCommandBuffers(state.device, &allocInfo,
                                        state.commandBuffers.data()),
               "vkAllocateCommandBuffers failed");
}

bool createSyncObjects(VulkanState& state) {
  VkSemaphoreCreateInfo semInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  if (!check(vkCreateSemaphore(state.device, &semInfo, nullptr,
                               &state.imageAvailable),
             "vkCreateSemaphore failed") ||
      !check(vkCreateSemaphore(state.device, &semInfo, nullptr,
                               &state.renderFinished),
             "vkCreateSemaphore failed") ||
      !check(vkCreateFence(state.device, &fenceInfo, nullptr, &state.inFlight),
             "vkCreateFence failed")) {
    return false;
  }
  return true;
}

void recordCommandBuffer(VulkanState& state, uint32_t imageIndex) {
  VkCommandBuffer cmd = state.commandBuffers[imageIndex];
  VkCommandBufferBeginInfo beginInfo{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  vkBeginCommandBuffer(cmd, &beginInfo);

  VkClearValue clear{};
  clear.color = {{0.05f, 0.05f, 0.08f, 1.0f}};

  VkRenderPassBeginInfo rpInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
  rpInfo.renderPass = state.renderPass;
  rpInfo.framebuffer = state.framebuffers[imageIndex];
  rpInfo.renderArea.offset = {0, 0};
  rpInfo.renderArea.extent = state.swapchainExtent;
  rpInfo.clearValueCount = 1;
  rpInfo.pClearValues = &clear;

  vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdEndRenderPass(cmd);
  vkEndCommandBuffer(cmd);
}

void cleanup(VulkanState& state) {
  if (state.device != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(state.device);

    vkDestroyFence(state.device, state.inFlight, nullptr);
    vkDestroySemaphore(state.device, state.renderFinished, nullptr);
    vkDestroySemaphore(state.device, state.imageAvailable, nullptr);

    for (auto fb : state.framebuffers) {
      vkDestroyFramebuffer(state.device, fb, nullptr);
    }
    vkDestroyRenderPass(state.device, state.renderPass, nullptr);

    for (auto view : state.imageViews) {
      vkDestroyImageView(state.device, view, nullptr);
    }

    vkDestroySwapchainKHR(state.device, state.swapchain, nullptr);
    vkDestroyCommandPool(state.device, state.commandPool, nullptr);
    vkDestroyDevice(state.device, nullptr);
  }
  if (state.surface != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(state.instance, state.surface, nullptr);
  }
  if (state.instance != VK_NULL_HANDLE) {
    vkDestroyInstance(state.instance, nullptr);
  }
}

}  // namespace

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
  const wchar_t kClassName[] = L"BlankVulkanWindow";
  WNDCLASS wc{};
  wc.lpfnWndProc = WindowProc;
  wc.hInstance = hInstance;
  wc.lpszClassName = kClassName;
  RegisterClass(&wc);

  HWND hwnd = CreateWindowEx(
      0, kClassName, L"Blank Vulkan Sample", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
      CW_USEDEFAULT, kWidth, kHeight, nullptr, nullptr, hInstance, nullptr);
  if (!hwnd) {
    return 1;
  }

  ShowWindow(hwnd, SW_SHOW);

  VulkanState state;
  if (!createInstance(state) || !createSurface(state, hInstance, hwnd) ||
      !pickPhysicalDevice(state) || !findQueueFamily(state) ||
      !createDevice(state) || !createSwapchain(state) ||
      !createRenderPass(state) || !createFramebuffers(state) ||
      !createCommandPool(state) || !createCommandBuffers(state) ||
      !createSyncObjects(state)) {
    cleanup(state);
    return 1;
  }

  MSG msg{};
  while (msg.message != WM_QUIT) {
    if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
      continue;
    }

    vkWaitForFences(state.device, 1, &state.inFlight, VK_TRUE, UINT64_MAX);
    vkResetFences(state.device, 1, &state.inFlight);

    uint32_t imageIndex = 0;
    vkAcquireNextImageKHR(state.device, state.swapchain, UINT64_MAX,
                          state.imageAvailable, VK_NULL_HANDLE, &imageIndex);

    vkResetCommandBuffer(state.commandBuffers[imageIndex], 0);
    recordCommandBuffer(state, imageIndex);

    VkPipelineStageFlags waitStage =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &state.imageAvailable;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &state.commandBuffers[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &state.renderFinished;

    vkQueueSubmit(state.graphicsQueue, 1, &submitInfo, state.inFlight);

    VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &state.renderFinished;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &state.swapchain;
    presentInfo.pImageIndices = &imageIndex;
    vkQueuePresentKHR(state.graphicsQueue, &presentInfo);
  }

  cleanup(state);
  DestroyWindow(hwnd);
  return 0;
}
