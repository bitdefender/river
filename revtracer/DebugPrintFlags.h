#ifndef _DEBUG_PRINT_FLAGS_H_
#define _DEBUG_PRINT_FLAGS_H_


// Enable this to understand what is happening on river side. Disabled now for performance reasons
#ifdef IS_DEBUG_BUILD
#define ENABLE_RIVER_SIDE_DEBUGGING
#endif

// message type
#define PRINT_ERROR					0x01000000
#define PRINT_INFO					0x02000000
#define PRINT_DEBUG					0x03000000
#define PRINT_MESSAGE_MASK			0x0F000000
#define PRINT_MESSAGE_SHIFT			24

// execution stages
#define PRINT_BRANCH_HANDLER		0x00100000
#define PRINT_DISASSEMBLY			0x00200000
#define PRINT_TRANSLATION			0x00300000
#define PRINT_ASSEMBLY				0x00400000
#define PRINT_RUNTIME				0x00500000
#define PRINT_INSPECTION			0x00600000
#define PRINT_CONTAINER				0x00700000
#define PRINT_EXECUTION_MASK		0x00F00000
#define PRINT_EXECUTION_SHIFT		20

// code type - only for translation and assembly
#define PRINT_NATIVE				0x00010000
#define PRINT_RIVER					0x00020000
#define PRINT_TRACKING				0x00030000
#define PRINT_SYMBOLIC				0x00040000
#define PRINT_CODE_TYPE_MASK		0x000F0000
#define PRINT_CODE_TYPE_SHIFT		16

// code direction, only for river, tracking and symbolic
#define PRINT_FORWARD				0x00001000
#define PRINT_BACKWARD				0x00002000
#define PRINT_CODE_DIRECTION_MASK	0x0000F000
#define PRINT_CODE_DIRECTION_SHIFT	12


#endif
