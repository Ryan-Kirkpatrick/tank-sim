void _tank_assert(int assertion, const char* assertion_src, const char* file, const char* function, unsigned int line);
void _tank_assert_m(int assertion, const char* assertion_src, const char* file, const char* function, unsigned int line,
                    const char* format, ...);

#define TANK_ASSERT(x)                                           \
    do {                                                         \
        _tank_assert((x), #x, __FILE__, __FUNCTION__, __LINE__); \
    } while (0)

#define TANK_ASSERT_M(x, fmt, ...)                                                       \
    do {                                                                                 \
        _tank_assert_m((x), #x, __FILE__, __FUNCTION__, __LINE__, (fmt), ##__VA_ARGS__); \
    } while (0)
