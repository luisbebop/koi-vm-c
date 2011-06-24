#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define Int16 signed short
#define Word unsigned short
#define Int32 int
#define Bool unsigned char
#define Byte unsigned char
#define TRUE 1
#define FALSE 0

#define MAKEWORD(a, b)      ((Word)(((Byte)((a) & 0xff)) | ((Word)((Byte)((b) & 0xff))) << 8))

/* setup endianness */
#define HOSTISLITTLEENDIAN

#ifdef HOSTISBIGENDIAN
/* Host is BIG ENDIAN Architecture */
#define _SWAPS(x) 	((short)( \
((((short) x) & 0x000000FF) << 8) | \
((((short) x) & 0x0000FF00) >> 8)   \
))
#define _SWAPI(x) 	((int)( \
((((int) x) & 0x000000FF) << 24) | \
((((int) x) & 0x0000FF00) <<  8) | \
((((int) x) & 0x00FF0000) >>  8) | \
((((int) x) & 0xFF000000) >> 24) \
))
#define HTOLI(x) 	(_SWAPI(x))
#else
/* Host is NOT BIG ENDIAN Architecture */
/* Checks for LITTLE ENDIAN define */
#ifndef HOSTISLITTLEENDIAN
#error "You must define HOSTISBIGENDIAN or HOSTISLITTLEENDIAN!"
#endif
#define _SWAPS(x) (((short) x))
#define _SWAPI(x) (((int) x))
#endif

#define STACKSIZE 100

/* etype's from StackElement */
#define NIL_		0
#define BOOL_		1
#define INTEGER_	2
#define FLOAT_		3
#define	STRING_		4
#define	HASH_		5
#define FUNCTION_	16

typedef struct selement {
    int etype;
	int size;
    union { 
        int ival; 
        float fval;
        unsigned char cval;
        unsigned char *pval;
    } element;
} StackElement;

typedef struct snodeelement *StackNodeElementPtr;
typedef struct snodeelement
{
	StackElement data;
	StackNodeElementPtr link;
} StackNodeElement;

StackNodeElementPtr vm_data_stack_top = NULL;
StackNodeElementPtr vm_local_data_stack_top = NULL;

StackNodeElementPtr stack_init_dym()
{
	StackNodeElementPtr first_node;
	
	first_node = (StackNodeElementPtr)malloc(sizeof(StackNodeElement));
	if (first_node) 
	{
		first_node->link = NULL;
		return(first_node);
	}
	else 
	{
		printf("Memory Out of Space");
		return NULL;
	}
}

void stack_push_dym(StackNodeElementPtr top, StackElement value)
{
	StackNodeElementPtr x;
	x = (StackNodeElementPtr)malloc(sizeof(StackNodeElement));
	if (x == NULL) 
	{
		printf("Memory Out of Space");
        return;
	}
	else 
	{
		x->data = value;
		x->link = top->link;
		top->link = x;
	}
}

StackElement stack_pop_dym(StackNodeElementPtr top)
{
	StackElement a;
	StackNodeElementPtr p;
	
	if(top->link == NULL)
	{
		a.etype = -1;
		printf("Stack Empty ...");
	}
	else 
	{
		a = vm_data_stack_top->link->data;
		p = vm_data_stack_top->link;
		vm_data_stack_top->link = p->link;
		free(p);
	}
	return (a);
}

typedef struct stack *StackPtr;
typedef struct stack {
    int top; 
    StackElement items[STACKSIZE];
}Stack;

typedef struct istack *Int32StackPtr;
typedef struct istack
{
	Int32 top;
	Int32 items[STACKSIZE];
}Int32Stack;

Bool push_string_to_stack_from_tabledata(int index_tabledata);
void ins_gets(void);
void ins_push_string(void);
void ins_set_local(void);
void ins_get_local(void);
void ins_return(void);
void ins_call(void);
void ins_print(void);
void ins_push_function(void);
void define_vm_instructions(void);
Int32 load_file_in_opcodes_memory(char * filename);

Bool stack_is_empty(StackPtr ps);
StackElement stack_pop(StackPtr ps);
void stack_push(StackPtr ps, StackElement value);
Bool int_stack_is_empty(Int32StackPtr ps);
Int32 int_stack_pop(Int32StackPtr ps);
void int_stack_push(Int32StackPtr ps, Int32 value);

void vm_run(Int32 opcode_size);
/* end .h */

/* vm instructions */
#define PUSH_STRING		4
#define PRINT			120
#define GETS			121
#define SET_LOCAL		140
#define GET_LOCAL		141
#define PUSH_FUNCTION	160
#define END_FUNCTION	161
#define CALL			162
#define	RETURN			163

/* vm globals */
/*
 TO-DO:
 1 - Change all elements to dynamic memory allocation using wmalloc from Danilo
 2 - Iterate along the array of byte codes produced by Koi Compiler function to
 generate the binary file for this VM, hints:
 - get the integer and simple:
 >> f = open('test','wb')
 => #<File:test>
 >> a =  [13].pack('i')
 => "\r\000\000\000"
 >> f <<  [13].pack('i')
 => #<File:test>
 >> f.tell
 => 4
 >> f.close
 => nil
 >> quit
 - change strings to allocate on the end of file
 - change :symbols from variables to a sequence number starting in 0
 */

Int32 instruction_pointer;
Int32 opcodes[200 * 1024];
Byte tabledata[200 * 1024];
void (*instruction[200])(void);
Stack vm_data_stack;
Stack vm_local_data_stack;
Int32Stack vm_return_stack;

Bool stack_is_empty(StackPtr ps)
{
	if (ps->top == -1)
        return TRUE;
    else
        return FALSE;
}

StackElement stack_pop(StackPtr ps)
{
    StackElement element;
	
	element.etype = -1;
	if (stack_is_empty(ps))
    {
        return element;
    }
    return ps->items[ps->top--];
}

void stack_push(StackPtr ps, StackElement value)
{
    ps->items[++(ps->top)] = value;
}

Bool int_stack_is_empty(Int32StackPtr ps)
{
	if (ps->top == -1)
        return TRUE;
    else
        return FALSE;
}

Int32 int_stack_pop(Int32StackPtr ps)
{
	if (int_stack_is_empty(ps))
    {
        return -1;
    }
    return ps->items[ps->top--];
}

void int_stack_push(Int32StackPtr ps, Int32 value)
{
    ps->items[++(ps->top)] = value;
}

Bool push_string_to_stack_from_tabledata(int index_tabledata)
{
	Word size_string = 0;
	StackElement element;
	
	/* tabledata position is not string or binary data */
	if (tabledata[index_tabledata] != 0xFF) return FALSE;
	
	/* get size of data string or binary data */
	/* endianess problem ??? */
	size_string = MAKEWORD(tabledata[index_tabledata+1],tabledata[index_tabledata+2]);
	
	element.etype = STRING_;
	element.size = size_string;
	element.element.pval = &tabledata[index_tabledata+3];
	stack_push(&vm_data_stack,element);
	return TRUE;
}

void ins_gets(void)
{
	char buf[20];
	StackElement element;

	memset(buf,0,sizeof(buf));
	gets(buf);
	
	element.etype = STRING_;
	element.size = strlen(buf);
	element.element.pval = malloc(element.size);
	memcpy(element.element.pval, buf,element.size);
	stack_push(&vm_data_stack,element);
	instruction_pointer += 1;
	
	/*
	 @@instruction[GETS] = Proc.new() do |vm|
	 vm.data_stack.push([STRING_, $stdin.gets("\n")])
	 vm.instruction_pointer = vm.instruction_pointer + 1
	 end
	 */
}

void ins_push_string(void)
{
	Int32 index_tabledata = 0;
	
	index_tabledata = _SWAPI(opcodes[instruction_pointer+1]);
	push_string_to_stack_from_tabledata(index_tabledata);
	instruction_pointer += 2;
	
	/*
	 @@instruction[PUSH_STRING] = Proc.new() do |vm|
	 operand = vm.opcodes[vm.instruction_pointer + 1]
	 raise OperandError, "Expecting string value but got #{operand.inspect}" unless(operand.is_a?(String))
	 vm.data_stack.push([STRING_, operand])
	 vm.instruction_pointer = vm.instruction_pointer + 2
	 end
	 */
}

void ins_push_function(void)
{
	StackElement element;
	Int32 function_id = 0;
	Int32 i = 0;
	
	element.etype = FUNCTION_;
	element.element.ival = instruction_pointer + 2;
	stack_push(&vm_data_stack,element);	
	function_id = _SWAPI(opcodes[instruction_pointer + 1]);
	for (i = instruction_pointer; 42; i += 1) {
		if (_SWAPI(opcodes[i]) == END_FUNCTION && _SWAPI(opcodes[i+1]) == 0 && _SWAPI(opcodes[i+2]) == END_FUNCTION && _SWAPI(opcodes[i+3]) == function_id) 
		{
			break;
		}
	}
	instruction_pointer = i + 4;
	
	/*
	@@instruction[PUSH_FUNCTION] = Proc.new() do |vm|
		vm.data_stack.push([FUNCTION_, vm.instruction_pointer + 2])
		function_id = vm.opcodes[vm.instruction_pointer + 1]
		while(true)
			vm.instruction_pointer = vm.opcodes.slice(vm.instruction_pointer, vm.opcodes.length).index(END_FUNCTION) + vm.instruction_pointer
			if(vm.opcodes.slice(vm.instruction_pointer, 4) == [END_FUNCTION, 0, END_FUNCTION, function_id])
				break
			else
				vm.instruction_pointer = vm.instruction_pointer + 1
			end
		end
		vm.instruction_pointer = vm.instruction_pointer + 4
	end
	*/
}

void ins_set_local(void)
{
	vm_local_data_stack.items[_SWAPI(opcodes[instruction_pointer+1])] = stack_pop(&vm_data_stack);
	instruction_pointer = instruction_pointer + 2;
	/*
	@@instruction[SET_LOCAL] = Proc.new() do |vm|
		raise StackError, "Expecting at least one item on the data stack" unless(vm.data_stack.length > 0)
		vm.locals[vm.opcodes[vm.instruction_pointer + 1]] = vm.data_stack.pop
		vm.instruction_pointer = vm.instruction_pointer + 2
	end
	*/
}

void ins_get_local(void)
{
	StackElement element;
	
	element = vm_local_data_stack.items[_SWAPI(opcodes[instruction_pointer + 1])];
	stack_push(&vm_data_stack,element);
	instruction_pointer  = instruction_pointer + 2;
	/*
	@@instruction[GET_LOCAL] = Proc.new() do |vm|
		vm.data_stack.push( vm.locals[vm.opcodes[vm.instruction_pointer + 1]] )
		vm.instruction_pointer = vm.instruction_pointer + 2
	end
	*/
}

void ins_return(void)
{
	instruction_pointer = int_stack_pop(&vm_return_stack);
	/*
	@@instruction[RETURN] = Proc.new() do |vm|
		raise ReturnStackError, "Expecting at least one item on the return stack" unless(vm.return_stack.length > 0)
		vm.instruction_pointer = vm.return_stack.pop
		vm.decrement_scope
	end
	*/
}

void ins_call(void)
{
	StackElement element;
	
	int_stack_push(&vm_return_stack, instruction_pointer + 1);
	element = stack_pop(&vm_data_stack);
	instruction_pointer = element.element.ival;
	/*
	@@instruction[CALL] = Proc.new() do |vm|
		raise StackError, "Expecting at least one item on the data stack" unless(vm.data_stack.length > 0)
		raise StackError, "Expecting function" unless(vm.data_stack[-1][0] == FUNCTION_)
		vm.return_stack.push( vm.instruction_pointer + 1 )
		vm.instruction_pointer = vm.data_stack.pop[1]
		vm.increment_scope
	end
	*/
}

void ins_print(void)
{
	StackElement element;
	char buf[256]; //change to malloc based on element.size;
	
	memset(buf,0,sizeof(buf));
	element = stack_pop(&vm_data_stack);
	memcpy(buf,element.element.pval, element.size);
	printf("%s\n",buf);
	instruction_pointer += 1;
	
	/*
	 @@instruction[PRINT] = Proc.new() do |vm|
	 raise StackError, "Expecting at least one item on the data stack" unless(vm.data_stack.length > 0)
	 value = vm.data_stack.pop
	 raise StackError, "Expecting topmost stack item to be a string but was #{value.class}" unless(value[0] == STRING_)
	 print value[1]
	 vm.instruction_pointer = vm.instruction_pointer + 1
	 end
	 */
}

void define_vm_instructions(void)
{
	instruction[PUSH_STRING] = ins_push_string;
	instruction[PRINT] = ins_print;
	instruction[GETS] = ins_gets;
	instruction[PUSH_FUNCTION] = ins_push_function;
	instruction[GET_LOCAL] = ins_get_local;
	instruction[SET_LOCAL] = ins_set_local;
	instruction[CALL] = ins_call;
	instruction[RETURN] = ins_return;
}

Int32 load_file_in_opcodes_memory(char * filename)
{
    FILE * fp;
	Int32 num_opcodes = 0;
	Int32 size_data = 0;
	
    memset(opcodes, 0, sizeof(opcodes));
    fp = fopen(filename,"rb");
    if (fp)
    {
        /* read the first 4 bytes of file as Int32 to get the numbers of opcodes */
		fread(&num_opcodes, 1, sizeof(Int32), fp);
		/* convert num_opcodes if the host is little or big endian, because the memory align is different between the platforms */
		num_opcodes = _SWAPI(num_opcodes);
		/* read all opcodes in opcodes array */
        fread(&opcodes, num_opcodes, sizeof(Int32),fp);
		/* read the size of tabledata */
		fread(&size_data,1,sizeof(Int32),fp);
		/* endianness*/
		size_data = _SWAPI(size_data);
		/* read all tabledata */
		fread(&tabledata,size_data,sizeof(Byte),fp);
        fclose(fp);
		/* load vm instructions */
		define_vm_instructions();
        return num_opcodes;
    }
    else
    {
        return -1;
    }
}

void vm_run(Int32 opcode_size)
{
	instruction_pointer = 0;
	while (instruction_pointer < opcode_size)
	{
		//call to function pointer
		(*instruction[_SWAPI(opcodes[instruction_pointer])])();
	}
}

int main_old()
{
	Int32 opcode_size = 0;
	opcode_size = load_file_in_opcodes_memory("helloworld.exe");
	vm_run(opcode_size);
	return 0;
}

int main()
{
	StackElement s, z, a, b, c;
	
	vm_data_stack_top = stack_init_dym();
	
	s.etype = INTEGER_;
	s.element.ival = 666;
	stack_push_dym(vm_data_stack_top, s);
	
	z.etype = INTEGER_;
	z.element.ival = 777;
	stack_push_dym(vm_data_stack_top, z);
	
	a = stack_pop_dym(vm_data_stack_top);
	b = stack_pop_dym(vm_data_stack_top);
	c = stack_pop_dym(vm_data_stack_top);
}
