#define MAX_FRAMES_IN_FLIGHT (uint32_t)2

#define IF_THROW(expr, message) \
  if ((expr)) { \
    throw std::runtime_error(#message); \
  }

#define CHECK_NULL(expr) \
    if (!(expr)) { \
      throw std::runtime_error(#expr " is nullptr..."); \
    }
