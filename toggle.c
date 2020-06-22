/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_subscriptions.h>
#include <open62541/plugin/log_stdout.h>

#include "../common.h"

static void usage(void) {
    printf("Usage: client [-username name] [-password password] ");
#ifdef UA_ENABLE_ENCRYPTION
    printf("[-cert certfile.der] [-key keyfile.der] "
           "[-securityMode <0-3>] [-securityPolicy policyUri] ");
#endif
    printf("opc.tcp://<host>:<port> -node_str # [-value #####]\n");
}

int HandleArguments(
    int argc, char *argv[],
    char **serverurl, char **username, char **password, 
    char **certfile, char **keyfile, char **securityPolicy,
    char **node_str, char **namespace, char **value, bool *IsBrowse)
{

    /* At least one argument is required for the server uri */
    if(argc <= 1) {
        usage();
        return EXIT_FAILURE;
    }

    /* Parse the arguments */
    for(int argpos = 1; argpos < argc; argpos++) {
        if(strcmp(argv[argpos], "--help") == 0 ||
           strcmp(argv[argpos], "-h") == 0) {
            usage();
            return EXIT_FAILURE;
        }

        if(argpos + 1 == argc) {
            *serverurl = argv[argpos];
            continue;
        }

        if(strcmp(argv[argpos], "-node") == 0) {
            argpos++;
            *node_str = argv[argpos];
            continue;
        }

        if(strcmp(argv[argpos], "-namespace") == 0) {
            argpos++;
            *namespace = argv[argpos];
            continue;
        }

        if(strcmp(argv[argpos], "-value") == 0) {
            argpos++;
            *value = argv[argpos];
            continue;
        }

        if(strcmp(argv[argpos], "-browse") == 0) {
            *IsBrowse = true;
            continue;
        }

        if(strcmp(argv[argpos], "-username") == 0) {
            argpos++;
            *username = argv[argpos];
            continue;
        }

        if(strcmp(argv[argpos], "-password") == 0) {
            argpos++;
            *password = argv[argpos];
            continue;
        }

#ifdef UA_ENABLE_ENCRYPTION
        if(strcmp(argv[argpos], "-cert") == 0) {
            argpos++;
            *certfile = argv[argpos];
            continue;
        }

        if(strcmp(argv[argpos], "-key") == 0) {
            argpos++;
            *keyfile = argv[argpos];
            continue;
        }

        if(strcmp(argv[argpos], "-securityMode") == 0) {
            argpos++;
            if(sscanf(argv[argpos], "%i", (int*)&securityMode) != 1) {
                usage();
                return EXIT_FAILURE;
            }
            continue;
        }

        if(strcmp(argv[argpos], "-securityPolicy") == 0) {
            argpos++;
            *securityPolicyUri = UA_String_fromChars(argv[argpos]);
            continue;
        }
#endif
        usage();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

UA_StatusCode ConnectClient(char *serverurl, char *username, char *password,
                         char *certfile, char *keyfile, char *securityPolicy, UA_Client *client){
    UA_String securityPolicyUri = UA_STRING_NULL; //we need to map the char to the var
    UA_MessageSecurityMode securityMode = UA_MESSAGESECURITYMODE_INVALID; /* allow everything */
    /* Create the server and set its config */
    UA_ClientConfig *cc = UA_Client_getConfig(client);

#ifdef UA_ENABLE_ENCRYPTION
    if(certfile) {
        UA_ByteString certificate = loadFile(certfile);
        UA_ByteString privateKey  = loadFile(keyfile);
        UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey, NULL, 0, NULL, 0);
        UA_ByteString_clear(&certificate);
        UA_ByteString_clear(&privateKey);
    } else {
        UA_ClientConfig_setDefault(cc);
    }
#else
    UA_ClientConfig_setDefault(cc);
#endif

    cc->securityMode = securityMode;
    cc->securityPolicyUri = securityPolicyUri;

    /* Connect to the server */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(username)
        retval = UA_Client_connectUsername(client, serverurl, username, password);
    else
        retval = UA_Client_connect(client, serverurl);
        
    if(retval != UA_STATUSCODE_GOOD)
        UA_Client_delete(client);
    else
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Connected!");

    return retval;
}

UA_NodeId *FindNode(char *namespace, char *Node_Str){
    UA_NodeId *Node = UA_NodeId_new();

    if (atoi(Node_Str) == 0 ) 
        *Node = UA_NODEID_STRING_ALLOC(atoi(namespace), Node_Str);
    else 
        *Node = UA_NODEID_NUMERIC(atoi(namespace), atoi(Node_Str));
    return Node;
};

static UA_StatusCode
nodeIter(UA_NodeId childId, UA_Boolean isInverse, UA_NodeId referenceTypeId, void *handle) {
    if(isInverse || (referenceTypeId.identifier.numeric == 40)) 
        return UA_STATUSCODE_GOOD;
         UA_NodeId *parent = (UA_NodeId *)handle;
    printf("UA_NODEID{\"Id\":%d,\"NameSpace\":%i,\"Type\":%d,\"Parent\":%d,\"Parent_NameSpace\":%i}UA_NODEID\n",
        childId.identifier.numeric, childId.namespaceIndex,
        referenceTypeId.identifier.numeric, parent->namespaceIndex, parent->identifier.numeric);
           
    return UA_STATUSCODE_GOOD;
};

UA_StatusCode Handle_Values(UA_Client *client, UA_NodeId *Node, char *value)
{
    UA_LocalizedText *display  = UA_LocalizedText_new();
    UA_QualifiedName *qualified= UA_QualifiedName_new();
    UA_NodeClass     *class    = UA_NodeClass_new();
    UA_StatusCode retval;
    UA_Variant val;
    UA_Variant_init(&val);    
    char Response[1000];

    retval = UA_Client_readDisplayNameAttribute(client, *Node, display);
    if(retval == UA_STATUSCODE_GOOD){
        sprintf(Response,"%s\"DisplayName\":\"%s\",", Response,display->text.data);
        retval = UA_Client_readDescriptionAttribute(client, *Node, display);
        sprintf(Response,"%s\"Description\":\"%s\",", Response,display->text.data);
        retval = UA_Client_readBrowseNameAttribute(client, *Node, qualified);
        sprintf(Response,"%s\"BrowseName\":\"%s\",",Response,qualified->name.data);
        retval = UA_Client_readNodeClassAttribute(client, *Node, class);
        sprintf(Response,"%s\"NodeClass\":\"%s\"",Response,display->text.data);
        retval = UA_Client_readValueAttribute(client, *Node, &val);
        
        if(retval == UA_STATUSCODE_GOOD && UA_Variant_isScalar(&val)) {
            switch(val.type->typeIndex) {
                case UA_TYPES_BOOLEAN:
                    if (value != NULL) {
                        sprintf(Response,"%s,\"Initial\":%s",Response, *(UA_Boolean*)val.data ? "true" : "false");
                        UA_Boolean BValue = (strcmp(value,"true") == 0);
                        sprintf(Response,"%s,\"Desired\":%s",Response, ( BValue ? "true" : "false"));
                        UA_Variant_setScalarCopy(&val, &BValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
                        retval = UA_Client_writeValueAttribute(client, *Node, &val);
                        UA_StatusCode retval = UA_Client_readValueAttribute(client, *Node, &val);
                    };
                    sprintf(Response,"%s,\"Current\":%s",Response, (*(UA_Boolean*)val.data) ? "true" : "false");
                    break; /* optional */
                case UA_TYPES_DOUBLE:
                    if (value != NULL) {
                        char *ptr;
                        UA_Double DValue = strtod(value, &ptr);
                        sprintf(Response,"%s\"Initial\":%f,",Response,  *(UA_Double*)val.data);
                        sprintf(Response,"%s\"Desired\":%f,",Response, DValue);
                        UA_Variant_setScalar(&val, &DValue, &UA_TYPES[UA_TYPES_DOUBLE]);
                        retval = UA_Client_writeValueAttribute(client, *Node, &val);
                        UA_StatusCode retval = UA_Client_readValueAttribute(client, *Node, &val);
                    };
                    sprintf(Response,"%s\"Current\":%f", Response, *(UA_Double*)val.data);
                    break; /* optional */
                /* you can have any number of case statements */
                default : /* Optional */
                    sprintf(Response,"%s\"Error No Result}", Response);
            };
            if (value != NULL){
                retval = UA_Client_writeValueAttribute(client, *Node, &val);
                if(retval == UA_STATUSCODE_GOOD) {
                    printf("Successfully written.\n");
                }
                else {
                    printf("Error: writing value!\n");
                }
            }
        }
        else {
            printf("Unknown NodeId!\n\n\n");
        };
    };
    UA_Variant_clear(&val);
    printf("UA_Response{%s}UA_Response",Response);
    return retval;
};

int main(int argc, char *argv[]) {
    UA_Client *client = UA_Client_new();
    UA_NodeId *Node = UA_NodeId_new();

    char *serverurl = NULL;
    char *username = NULL;
    char *password = NULL;
    char *node_str = NULL;
    char *namespace = NULL;
    char *value = NULL;
    bool IsBrowse = false;
    char *certfile = NULL;
    char *keyfile = NULL;
    char *securityPolicy = NULL;
    
    if( HandleArguments(argc, argv, &serverurl, &username, &password, &certfile, &keyfile, &securityPolicy,
    &node_str, &namespace, &value, &IsBrowse) != EXIT_SUCCESS) return EXIT_FAILURE;

    if (ConnectClient(serverurl, username, password, certfile, keyfile, securityPolicy, client)
        != UA_STATUSCODE_GOOD) return EXIT_FAILURE;

    Node = FindNode(namespace, node_str);
    if (UA_NodeId_equal(Node, &UA_NODEID_NULL)) return EXIT_FAILURE;

    if (IsBrowse){
        UA_Client_forEachChildNodeCall(client, *Node, nodeIter, (void *) Node);
    }
    else 
        if (Handle_Values(client, Node, value) != UA_STATUSCODE_GOOD) return EXIT_FAILURE;

    UA_NodeId_clear(Node);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return EXIT_SUCCESS;
}
// htpasswd -c /var/opt/apache/passwd/passwords shawn
//
//vcgencmd measure_clock 

/*
arm:    frequency(45)=700000000
core:   frequency(1)=250000000
h264:   frequency(28)=0
isp:    frequency(42)=250000000
v3d:    frequency(43)=250000000
uart:   frequency(22)=3000000
pwm:    frequency(25)=0
emmc:   frequency(47)=100000000
pixel:  frequency(29)=154000000
vec:    frequency(10)=0
hdmi:   frequency(9)=163682000
dpi:    frequency(4)=0
*/
