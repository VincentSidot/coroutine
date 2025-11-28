
#ifndef _TESTS_UTILS_H
#define _TESTS_UTILS_H

#define ASSERT_TRUE(cond, msg)                                                 \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, (msg));         \
      return 1;                                                                \
    }                                                                          \
  } while (0)

#define ASSERT_EQ_INT(actual, expected, msg)                                   \
  ASSERT_TRUE((actual) == (expected), (msg))

#endif // _TESTS_UTILS_H
