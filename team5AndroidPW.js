const AWS = require("aws-sdk");
const ddb = new AWS.DynamoDB.DocumentClient();
const iotdata = new AWS.IotData({endpoint:"afkhvrjlvs6fo-ats.iot.ap-northeast-2.amazonaws.com"});

//안드로이드가 도어락 열기 버튼을 누르면 비밀번호를 전송 받아 스마트 도어락에 MQTT 메시지를 보낸다.
//스마트 도어락은 MQTT에 담긴 도어락 패스워드와 기존 도어락 EEPROM에 저장되어 있던 비밀번호를 비교한다.
exports.handler = (event, context ,callback) => {
    const openPW = event.PW
    var tempJson = {
        PW : openPW+"O"
    }
    tempJson = JSON.stringify(tempJson)
    // const tempJson = "{Flag: ON, PW:"+ PW+"}"
    const mqttParams = {
        topic: "team5/Door",
        payload: tempJson,
        qos: 0
        }
    iotdata.publish(mqttParams, function(err,data){if(err){console.log(err)}})

    callback();
};
