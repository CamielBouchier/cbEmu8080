
#include <stdint.h>

static const int VTPARSE_STATE_CSI_ENTRY = 1;
static const int VTPARSE_STATE_CSI_IGNORE = 2;
static const int VTPARSE_STATE_CSI_INTERMEDIATE = 3;
static const int VTPARSE_STATE_CSI_PARAM = 4;
static const int VTPARSE_STATE_DCS_ENTRY = 5;
static const int VTPARSE_STATE_DCS_IGNORE = 6;
static const int VTPARSE_STATE_DCS_INTERMEDIATE = 7;
static const int VTPARSE_STATE_DCS_PARAM = 8;
static const int VTPARSE_STATE_DCS_PASSTHROUGH = 9;
static const int VTPARSE_STATE_ESCAPE = 10;
static const int VTPARSE_STATE_ESCAPE_INTERMEDIATE = 11;
static const int VTPARSE_STATE_GROUND = 12;
static const int VTPARSE_STATE_OSC_STRING = 13;
static const int VTPARSE_STATE_SOS_PM_APC_STRING = 14;

static const int VTPARSE_ACTION_CLEAR = 1;
static const int VTPARSE_ACTION_COLLECT = 2;
static const int VTPARSE_ACTION_CSI_DISPATCH = 3;
static const int VTPARSE_ACTION_ESC_DISPATCH = 4;
static const int VTPARSE_ACTION_EXECUTE = 5;
static const int VTPARSE_ACTION_HOOK = 6;
static const int VTPARSE_ACTION_IGNORE = 7;
static const int VTPARSE_ACTION_OSC_END = 8;
static const int VTPARSE_ACTION_OSC_PUT = 9;
static const int VTPARSE_ACTION_OSC_START = 10;
static const int VTPARSE_ACTION_PARAM = 11;
static const int VTPARSE_ACTION_PRINT = 12;
static const int VTPARSE_ACTION_PUT = 13;
static const int VTPARSE_ACTION_UNHOOK = 14;
static const int VTPARSE_ACTION_ERROR = 15;

extern uint8_t STATE_TABLE[14][256];
extern int ENTRY_ACTIONS[14];
extern int EXIT_ACTIONS[14];
extern const char *ACTION_NAMES[16];
extern const char *STATE_NAMES[15];

