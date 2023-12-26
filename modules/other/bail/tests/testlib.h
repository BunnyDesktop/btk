#include <stdio.h>
#include <btk/btk.h>

/* Maximum characters in the output buffer */
#define MAX_LINE_SIZE   1000

/* Maximum number of tests */
#define MAX_TESTS       30 

/* Maximum number of test windows */
#define MAX_WINDOWS	5

/* Maximum number of parameters any test can have */
#define MAX_PARAMS      3

/* Information on the Output Window */

typedef struct
{
  BtkWidget     *outputWindow;
  BtkTextBuffer *outputBuffer; 
  BtkTextIter   outputIter;
}OutputWindow;

typedef void (*TLruntest) (BatkObject * obj, bint win_num);

/* General purpose functions */

bboolean		already_accessed_batk_object	(BatkObject	*obj);
BatkObject*		find_object_by_role		(BatkObject	*obj,
							BatkRole		*role,
							bint		num_roles);
BatkObject*		find_object_by_type		(BatkObject	*obj,
							bchar		*type);
BatkObject*		find_object_by_name_and_role	(BatkObject	*obj,
						      	const bchar	*name,
							BatkRole		*roles,
							bint		num_roles);
BatkObject*		find_object_by_accessible_name_and_role (BatkObject *obj,
							const bchar	*name,
							BatkRole		*roles,
							bint		num_roles);
void			display_children		(BatkObject	*obj,
                                                        bint		depth,
                                                        bint		child_number);
void			display_children_to_depth	(BatkObject	*obj,
                                                        bint		to_depth,
                                                        bint		depth,
                                                        bint		child_number);


/* Test GUI functions */

bint			create_windows			(BatkObject	*obj,
							TLruntest	runtest,
							OutputWindow	**outwin);
bboolean		add_test			(bint		window,
							bchar 		*name,
							bint		num_params,
							bchar 		*parameter_names[],
							bchar 		*default_names[]);
void			set_output_buffer		(bchar 		*output);
bchar			**tests_set			(bint		window,
							int		*count);
bchar			*get_arg_of_func		(bint		window,
							bchar		*function_name,
							bchar 		*arg_label);
int			string_to_int			(const char	*the_string);
bboolean		isVisibleDialog			(void);

