#include <deque>
struct DeletionQueue {
  std::deque<std::function<void()>> deletors;
  void push_function(std::function<void()> &&function) {
    deletors.push_back(function);
  }

  // Doing callbacks like this is inneficient at scale, because we are storing whole std::functions for every object we are deleting, which is not going to be optimal. For the amount of objects we will use in this tutorial, its going to be fine. but if you need to delete thousands of objects and want them deleted faster, a better implementation would be to store arrays of vulkan handles of various types such as VkImage, VkBuffer, and so on. And then delete those from a loop.
  void flush() {
    // reverse iterate the deletion queue to execute all the functions
    for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
      (*it)();
    }
    deletors.clear();
  }
};
