import { DynamoDBClient } from "@aws-sdk/client-dynamodb";
import { PutCommand, DynamoDBDocumentClient } from "@aws-sdk/lib-dynamodb";

const client = new DynamoDBClient({});
const docClient = DynamoDBDocumentClient.from(client);

export const handler = async (event, context) => {
    console.log('Received event:', JSON.stringify(event, null, 2));
    const command = new PutCommand({
        TableName: "ReaderEvents",
        Item: {
            "serialNumber": event.serialNumber,
            "timestamp": event.timestamp,
        },
    });
    const response = await docClient.send(command);
    console.log(response);
    return response;
};
