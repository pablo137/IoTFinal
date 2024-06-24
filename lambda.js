const Alexa = require('ask-sdk-core');
const AWS = require('aws-sdk');
const endpoint = process.env.IOT_ENDPOINT;
const IotData = new AWS.IotData({ endpoint: endpoint });
const DynamoDB = new AWS.DynamoDB.DocumentClient();
const iotClient = new AWS.Iot();
const TABLE_NAME = process.env.TABLE_NAME_DynamoDB;
let currentFoodStatus = ''; // Variable global para almacenar el estado de la comida

// Función para verificar si el userId está en DynamoDB
const isUserAuthorized = async (userId) => {
    const params = {
        TableName: TABLE_NAME,
        Key: {
            userId: userId
        }
    };

    try {
        const data = await DynamoDB.get(params).promise();
        return data.Item !== undefined;
    } catch (error) {
        console.error('Error al consultar DynamoDB:', error);
        return false;
    }
};

// Función para verificar autorización y responder en consecuencia
const verifyAuthorization = async (handlerInput) => {
    const userId = handlerInput.requestEnvelope.session.user.userId;
    //const userId = "sdsa432423dfasdfsdafas3";

    // Verificar si el usuario está autorizado
    const authorized = await isUserAuthorized(userId);
    if (!authorized) {
        const speakOutput = 'Lo siento, no estás autorizado para usar esta skill.';
        return handlerInput.responseBuilder
            .speak(speakOutput)
            .getResponse();
    }
    return null;
};

const handleFoodStatus = async (status) => {
    if (status === "noFood") {
        console.log("No food in the plate.");
    } else if (status === "withFood") {
        console.log("Food is available in the plate.");
    } else {
        console.log("Unknown food status.");
    }
};

const MessageHandler = async (event) => {
    console.log("Received event:", JSON.stringify(event, null, 2));

    try {
        // Extraer food_status del evento
        const foodStatus = event.state.reported.food_status;
        console.log("Food status:", foodStatus);

        // Actualizar el estado de la comida
        currentFoodStatus = foodStatus;

        // Manejar el estado de food_status
        await handleFoodStatus(foodStatus);
    } catch (error) {
        console.error("Error processing event:", error);
    }

    return {
        statusCode: 200,
        body: JSON.stringify('Lambda executed successfully'),
    };
};


// LaunchRequestHandler
const LaunchRequestHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'LaunchRequest';
    },
    async handle(handlerInput) {
        const authorizationResponse = await verifyAuthorization(handlerInput);
        if (authorizationResponse) {
            return authorizationResponse;
        }

        const speakOutput = 'Bienvenido al gato feeder 1, puedes pedir ayuda!';

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .reprompt(speakOutput)
            .getResponse();
    }
};

// Intento para solicitar el estado de la comida
const FoodStatusIntentHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest' &&
            Alexa.getIntentName(handlerInput.requestEnvelope) === 'FoodStatusIntent';
    },
    handle(handlerInput) {
        let speakOutput = 'Solicitando el estado de la comida...';

        const params = {
            topic: 'gato_distancia_test',
            payload: JSON.stringify({
                state: {
                    desired: {
                        result: 'food?',
                        origin: 'alexa'
                    }
                }
            }),
            qos: 0
        };

        IotData.publish(params, (err, data) => {
            if (err) {
                console.error('Error al publicar en el tópico:', err);
                speakOutput = 'Lo siento, no pude solicitar el estado de la comida.';
            } else {
                console.log('Mensaje publicado exitosamente:', data);
            }
        });

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .reprompt(speakOutput)
            .getResponse();
    }
};

// Intento para verificar el estado de la comida
const CheckFoodStatusIntentHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest' &&
            Alexa.getIntentName(handlerInput.requestEnvelope) === 'CheckFoodStatusIntent';
    },
    handle(handlerInput) {
        let speakOutput = '';

        // Obtener el estado de la comida de los atributos de la sesión
        const sessionAttributes = handlerInput.attributesManager.getSessionAttributes();
        const foodStatus = sessionAttributes.foodStatus;
        console.log("ACA ESTOY",foodStatus);
        // if (foodStatus === 'withFood') {
        //     speakOutput = 'El plato del gato tiene comida';
        // } else if (foodStatus === 'noFood') {
        //     speakOutput = 'El plato no tiene comida';
        // } else {
        //     speakOutput = 'No estoy segura sobre el estado del plato.';
        // }

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .reprompt('¿Necesitas algo más?')
            .getResponse();
    }
};


// Intento para girar el servomotor
const GirarServo = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest'
            && Alexa.getIntentName(handlerInput.requestEnvelope) === 'GirarServoIntent';
    },
    handle(handlerInput) {
        let speakOutput = 'Enviando datos al gato distancia test desde Alexa como usuario';
        const params = {
            topic: 'gato_distancia_test',
            payload: JSON.stringify({
                state: {
                    desired: {
                        result: 'giraServo',
                        origin: 'alexa'
                    }
                }
            }),
            qos: 0
        };

        IotData.publish(params, (err, data) => {
            if (err) {
                console.error('Error al publicar en el tópico:', err);
                speakOutput = 'Lo siento, no pude abrir el Internet de las Cosas';
            } else {
                console.log('Mensaje publicado exitosamente:', data);
            }
        });

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .reprompt(speakOutput)
            .getResponse();
    }
};

// Intento para solicitar el estado de la comida
const AutoStatusIntentHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest' &&
            Alexa.getIntentName(handlerInput.requestEnvelope) === 'AutoStatusIntent';
    },
    handle(handlerInput) {
        let speakOutput = 'Modo automatico activado...';

        const params = {
            topic: 'gato_distancia_test',
            payload: JSON.stringify({
                state: {
                    desired: {
                        result: 'modoAuto',
                        pub: 'on',
                        origin: 'alexa'
                    }
                }
            }),
            qos: 0
        };

        IotData.publish(params, (err, data) => {
            if (err) {
                console.error('Error al publicar en el tópico:', err);
                speakOutput = 'Lo siento, ocurrio algo innesperado.';
            } else {
                console.log('Mensaje publicado exitosamente:', data);
            }
        });

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .reprompt(speakOutput)
            .getResponse();
    }
};

// Intento para solicitar el estado de la comida
const OffAutoStatusIntentHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest' &&
            Alexa.getIntentName(handlerInput.requestEnvelope) === 'OffAutoStatusIntent';
    },
    handle(handlerInput) {
        let speakOutput = 'Modo automatico desactivado...';

        const params = {
            topic: 'gato_distancia_test',
            payload: JSON.stringify({
                state: {
                    desired: {
                        result: 'modoDesauto',
                        origin: 'alexa'
                    }
                }
            }),
            qos: 0
        };

        IotData.publish(params, (err, data) => {
            if (err) {
                console.error('Error al publicar en el tópico:', err);
                speakOutput = 'Lo siento, ocurrio algo innesperado.';
            } else {
                console.log('Mensaje publicado exitosamente:', data);
            }
        });

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .reprompt(speakOutput)
            .getResponse();
    }
};


// Intento para listar dispositivos conectados
const ListConnectedDevicesIntent = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest'
            && Alexa.getIntentName(handlerInput.requestEnvelope) === 'ConsultarDisposConectadosIntent';
    },
    async handle(handlerInput) {
        let speakOutput = 'Los dispositivos conectados son: ';
        try {
            const things = await iotClient.listThings().promise();
            const connectedDevices = [];

            for (const thing of things.things) {
                const thingName = thing.thingName;
                const shadow = await IotData.getThingShadow({ thingName }).promise();
                const shadowPayload = JSON.parse(shadow.payload);
                
                if (shadowPayload.state && shadowPayload.state.reported && shadowPayload.state.reported.connected) {
                    connectedDevices.push(thingName);
                }
            }

            if (connectedDevices.length > 0) {
                speakOutput += connectedDevices.join(', ');
            } else {
                speakOutput = 'No hay dispositivos conectados.';
            }
        } catch (error) {
            console.error('Error al listar los dispositivos conectados:', error);
            speakOutput = 'Lo siento, no pude obtener la lista de dispositivos conectados.';
        }

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .getResponse();
    }
};

// Manejar la recepción de mensajes MQTT desde IoT Core
const EscucharESPIntent = (message) => {
    const messageBody = JSON.parse(message);
    const result = messageBody.state.desired.result;

    if (result === 'dispensed') {
        const speakOutput = 'El servomotor está girando por el objeto inteligente';
        return speakOutput;
    }
};

// Handler para mensajes MQTT
exports.handler = async (event) => {
    const message = event.payload.toString('utf8');
    const response = EscucharESPIntent(message);

    if (response) {
        return {
            statusCode: 200,
            body: response
        };
    } else {
        return {
            statusCode: 200,
            body: 'Mensaje procesado'
        };
    }
};

// Handlers de cancelación y parada
const CancelAndStopIntentHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest'
            && (Alexa.getIntentName(handlerInput.requestEnvelope) === 'AMAZON.CancelIntent'
                || Alexa.getIntentName(handlerInput.requestEnvelope) === 'AMAZON.StopIntent');
    },
    handle(handlerInput) {
        const speakOutput = 'Adiós!';

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .getResponse();
    }
};

// Handler de Fallback
const FallbackIntentHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest'
            && Alexa.getIntentName(handlerInput.requestEnvelope) === 'AMAZON.FallbackIntent';
    },
    handle(handlerInput) {
        const speakOutput = 'Sorry, I don\'t know about that. Please try again.';

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .reprompt(speakOutput)
            .getResponse();
    }
};

// Handler de fin de sesión
const SessionEndedRequestHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'SessionEndedRequest';
    },
    handle(handlerInput) {
        console.log(`~~~~ Session ended: ${JSON.stringify(handlerInput.requestEnvelope)}`);
        return handlerInput.responseBuilder.getResponse();
    }
};

// Handler de reflejo de intentos
const IntentReflectorHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest';
    },
    handle(handlerInput) {
        const intentName = Alexa.getIntentName(handlerInput.requestEnvelope);
        const speakOutput = `You just triggered ${intentName}`;

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .getResponse();
    }
};

// Handler de errores
const ErrorHandler = {
    canHandle() {
        return true;
    },
    handle(handlerInput, error) {
        const speakOutput = 'Sorry, I had trouble doing what you asked. Please try again.';
        console.log(`~~~~ Error handled: ${JSON.stringify(error)}`);

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .reprompt(speakOutput)
            .getResponse();
    }
};

// Configuración de la skill
exports.handler = Alexa.SkillBuilders.custom()
    .addRequestHandlers(
        LaunchRequestHandler,
        FoodStatusIntentHandler,
        AutoStatusIntentHandler,
        OffAutoStatusIntentHandler,
        GirarServo,
        CheckFoodStatusIntentHandler,
        ListConnectedDevicesIntent,
        CancelAndStopIntentHandler,
        FallbackIntentHandler,
        SessionEndedRequestHandler,
        IntentReflectorHandler
    )
    .addErrorHandlers(
        ErrorHandler
    )
    .withCustomUserAgent('sample/hello-world/v1.2')
    .lambda();
