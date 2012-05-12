#ifndef UNITTEST_H
#define UNITTEST_H

#define UNITTEST(expr)					((expr) ? 1 : unittest_fail(__STRING(expr), __FILE__, __LINE__, __func__))

int unittest_fail(const char *expr, const char *file, unsigned int line, const char *func);

unsigned int random_int(void);

#endif /* UNITTEST_H */
