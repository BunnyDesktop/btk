#ifndef __TEST_PRINT_FILE_OPERATION_H__
#define __TEST_PRINT_FILE_OPERATION_H__

#include <btk/btk.h>

B_BEGIN_DECLS

#define TEST_TYPE_PRINT_FILE_OPERATION    (test_print_file_operation_get_type ())
#define TEST_PRINT_FILE_OPERATION(obj)    (B_TYPE_CHECK_INSTANCE_CAST ((obj), TEST_TYPE_PRINT_FILE_OPERATION, TestPrintFileOperation))
#define TEST_IS_PRINT_FILE_OPERATION(obj) (B_TYPE_CHECK_INSTANCE_TYPE ((obj), TEST_TYPE_PRINT_FILE_OPERATION))

typedef struct _TestPrintFileOperationClass   TestPrintFileOperationClass;
typedef struct _TestPrintFileOperationPrivate TestPrintFileOperationPrivate;
typedef struct _TestPrintFileOperation        TestPrintFileOperation;

struct _TestPrintFileOperation
{
  BtkPrintOperation parent_instance;

  /* < private > */
  char *filename;
  double font_size;
  int lines_per_page;

  
  char **lines;
  int num_lines;
  int num_pages;
};

struct _TestPrintFileOperationClass
{
  BtkPrintOperationClass parent_class;
};

GType                   test_print_file_operation_get_type      (void);
TestPrintFileOperation *test_print_file_operation_new           (const char             *filename);
void                    test_print_file_operation_set_font_size (TestPrintFileOperation *op,
								 double                  points);

B_END_DECLS

#endif /* __TEST_PRINT_FILE_OPERATION_H__ */
