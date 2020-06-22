#include <open62541.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)
#define BCM2708_PERI_BASE        0x3F000000
#define BCM2711_PERI_BASE        0xFE000000
#define GPIO_BASE                (BCM2711_PERI_BASE + 0x200000) /* GPIO controller */

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(gpio+((g)/10)) &= (unsigned int)~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(gpio+((g)/10)) |=  (unsigned int)(1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio+(((g)/10))) |= (unsigned int)(((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

#define GPIO_SET *(gpio+7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR *(gpio+10) // clears bits which are 1 ignores bits which are 0

#define GET_GPIO(g) (*(gpio+13)&(unsigned int)(1<<g)) // 0 if LOW, (1<<g) if HIGH

#define GPIO_PULL *(gpio+37)      // Pull up/pull down
#define GPIO_PULLCLK0 *(gpio+38)  // Pull up/pull down clock


int  mem_fd;
void *gpio_map;
volatile unsigned *gpio;

//  How to access GPIO registers from C-code on the Raspberry-Pi
//  Example program
//  15-January-2012
//  Dom and Gert
//  Revised: 15-Feb-2013

// Access from ARM Running Linux

static double FileDoubleInput(char *file)
{
	FILE *DoubleInt;
	double T;
		T= -10000;
	DoubleInt = fopen (file, "r");
	if (DoubleInt == NULL){
	  printf("%s file did not open!\n", file); //print some message
	}
	else {
		fscanf (DoubleInt, "%lf", &T);
		fclose (DoubleInt);
	}
	return T;
}

static bool StatusGPIO(int g){
    INP_GPIO(g); // must use INP_GPIO before we can use OUT_GPIO
    OUT_GPIO(g); 

    if (GET_GPIO(g) > 0){
        return true;
    }
    else {
        return false;
    };
}

static void SetGPIO(int g, bool status){
    INP_GPIO(g); // must use INP_GPIO before we can use OUT_GPIO
    OUT_GPIO(g); 

    if (status) {
        GPIO_SET = (unsigned int)1<<g;
    }
    else {
        GPIO_CLR = (unsigned int)1<<g;
    }
}

//
// Set up a memory regions to access GPIO
//
static void setup_io(void)
{
   /* open /dev/mem */
   if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
      printf("can't open /dev/mem \n");
      exit(-1);
   }

   /* mmap GPIO */
   gpio_map = mmap(
      NULL,             //Any adddress in our space will do
      BLOCK_SIZE,       //Map length
      PROT_READ|PROT_WRITE,// Enable reading & writting to mapped memory
      MAP_SHARED,       //Shared with other processes
      mem_fd,           //File to map
      (off_t) GPIO_BASE         //Offset to GPIO peripheral
   );

   close(mem_fd); //No need to keep mem_fd open after mmap

   if (gpio_map == MAP_FAILED) {
      printf("mmap error %d\n", (int)gpio_map);//errno also set!
      exit(-1);
   }

   // Always use volatile pointer!
   gpio = (volatile unsigned *)gpio_map;
} 
// setup_io

// Temp
static UA_StatusCode
readCurrentTemp(UA_Server *server,
                const UA_NodeId *sessionId, void *sessionContext,
                const UA_NodeId *nodeId, void *nodeContext,
                UA_Boolean sourceTempStamp, const UA_NumericRange *range,
                
                UA_DataValue *dataValue) {
    UA_LocalizedText outDescription;
    UA_Server_readDescription(server, *nodeId, &outDescription);

    UA_Double temp = FileDoubleInput(outDescription.text.data);
    UA_Variant_setScalarCopy(&dataValue->value, &temp, &UA_TYPES[UA_TYPES_DOUBLE]);
   
    dataValue->hasValue = true;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
writeCurrentTemp(UA_Server *server,
                 const UA_NodeId *sessionId, void *sessionContext,
                 const UA_NodeId *nodeId, void *nodeContext,
                 const UA_NumericRange *range, const UA_DataValue *data) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Changing the system temp is not implemented");
    return UA_STATUSCODE_BADINTERNALERROR;
}

static void
addFileToDouble(UA_Server *server, char *name, char *file) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", name);
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attr.description = UA_LOCALIZEDTEXT("en-US", file);

    UA_NodeId currentNodeId = UA_NODEID_NULL;
    UA_QualifiedName currentName = UA_QUALIFIEDNAME(1, name);
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId variableTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);
    UA_DataSource tempDataSource;
    tempDataSource.read = readCurrentTemp;
    tempDataSource.write = writeCurrentTemp;
    
    UA_Server_addDataSourceVariableNode(server, currentNodeId, parentNodeId,
                                        parentReferenceNodeId, currentName,
                                        variableTypeNodeId, attr,
                                        tempDataSource, NULL, NULL);
}
// Temp

// GPIO
static UA_StatusCode
readCurrentGPIO(UA_Server *server,
                const UA_NodeId *sessionId, void *sessionContext,
                const UA_NodeId *nodeId, void *nodeContext,
                UA_Boolean sourceTempStamp, const UA_NumericRange *range,                
                UA_DataValue *dataValue) {

    UA_LocalizedText outDescription;
    UA_Server_readDescription(server, *nodeId, &outDescription);
    int GPIOnum = atoi( (char *)outDescription.text.data );

    bool temp = StatusGPIO(GPIOnum);
    UA_Variant_setScalarCopy(&dataValue->value, &temp, &UA_TYPES[UA_TYPES_BOOLEAN]);
    dataValue->hasValue = true;
     
    char str[100];
    sprintf(str, "GPIO %s is %s.", outDescription.text.data, temp ? "true" : "false");
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, str);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
writeCurrentGPIO(UA_Server *server,
                 const UA_NodeId *sessionId, void *sessionContext,
                 const UA_NodeId *nodeId, void *nodeContext,
                 const UA_NumericRange *range, const UA_DataValue *dataValue) {

    UA_LocalizedText outDescription;
    UA_Server_readDescription(server, *nodeId, &outDescription);
    int GPIOnum = atoi( (char *)outDescription.text.data );

    char str[100];
    if(dataValue->value.type == &UA_TYPES[UA_TYPES_BOOLEAN]) {
        UA_Boolean gpio = *(UA_Boolean*)dataValue->value.data;
        sprintf(str,"Set GPIO %s to %s.", outDescription.text.data, gpio ? "true" : "false");
        SetGPIO(GPIOnum, gpio);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, str);
        return UA_STATUSCODE_GOOD;
    }
    else {
        sprintf(str, "%s was not set to %s.", outDescription.text.data, gpio ? "true" : "false");
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, str);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
}


// End 
/*
Add FileVariable
Remove FileVariable

Add GPIO
  Name
  displayName
  description
  Input Only
  GPIO Number
  Execute
Remove GPIO

Add WMI Client variable
Remove WMI

main() {
}


*/
/**
 * Now we add the derived ObjectType for the Gpio that inherits from the device
 * object type. The resulting object contains all mandatory child variables.
 * These are simply copied over from the object type. The object has a reference
 * of type ``hasTypeDefinition`` to the object type, so that clients can detect
 * the type-instance relation at runtime.
 */

static void
addGPIOObjectInstance(UA_Server *server, char *name, int GPIO) {
     char str[2];
    sprintf(str, "%d", GPIO);

    UA_DataSource gpioDataSource;
    gpioDataSource.read = readCurrentGPIO;
    gpioDataSource.write = writeCurrentGPIO;

    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", name);
    attr.description = UA_LOCALIZEDTEXT("en-US", str);
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Boolean status = StatusGPIO(GPIO);
    UA_Variant_setScalar(&attr.value, &status, &UA_TYPES[UA_TYPES_INT32]);

    UA_NodeId currentNodeId = UA_NODEID_NULL;
    UA_QualifiedName currentName = UA_QUALIFIEDNAME(1, name);
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId variableTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);

    UA_Server_addDataSourceVariableNode(server, currentNodeId, parentNodeId,
                                        parentReferenceNodeId, currentName,
                                        variableTypeNodeId, attr,
                                        gpioDataSource, NULL, NULL);

}

/** It follows the main server code, making use of the above definitions. */
static volatile UA_Boolean running = true;
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

int main(void) {

	setup_io();

    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig *Config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(Config);
    addFileToDouble(server,"CPU-Temp", "/sys/class/thermal/thermal_zone0/temp");
    addFileToDouble(server,"CPU-Freq", "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");
    addFileToDouble(server,"GPU-Temp", "/opt/vc/bin/vcgencmd measure_temp");
    int i;
    char str[10];
    for (i=2; i<=9; i++)
    {
        sprintf(str, "GPIO%d", i);
        printf("Adding %s\n",str);
        addGPIOObjectInstance(server, str,i);
    }

    UA_StatusCode retval = UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}

//gcc -std=c99 open62541.c RaspOPCUA.c -o RaspOPCUA -lmbedtls -lmbedx509 -lmbedcrypto
    //UA_ByteString certificate = loadFile("server/server.crt.der");
    //UA_ByteString privateKey = loadFile("server/server.key.der");
