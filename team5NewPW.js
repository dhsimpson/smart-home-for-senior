const AWS = require("aws-sdk");
const ddb = new AWS.DynamoDB.DocumentClient();
const iotdata = new AWS.IotData({endpoint:"afkhvrjlvs6fo-ats.iot.ap-northeast-2.amazonaws.com"});

exports.handler = (event, context ,callback) => {
    const newPW = event.PW

//안드로이드가 도어락 비밀번호 변경 버튼을 누르면 비밀번호를 전송 받아 DynamoDB에 비밀번호를 저장 시키고
//스마트 도어락에 MQTT 메시지를 보낸다.(테이블 명은 iot_project_DoorPW)
//스마트 도어락은 MQTT에 담긴 도어락 패스워드를 EEPROM과 비밀번호 배열에 저장한다.
    ddb.scan({TableName: "iot_project_DoorPW"},function(err,data){
        if(err){console.log(err)}
        else{
            var idx = data["Count"]
            const params = {
                TableName: "iot_project_DoorPW",
                Item : {
                    idx: idx+1,
                    newPW: newPW
                }
            }
            ddb.put(params,function(err,data){
                if(err){}
                else{
                    var tempJson = {
                        PW : newPW+"C"
                    }
                    tempJson = JSON.stringify(tempJson)
                    const mqttParams = {
                        topic: "team5/Door",
                        payload: tempJson,
                        qos: 0
                    }
                    iotdata.publish(mqttParams, function(err,data){if(err){console.log(err)}})
                }
            });
        }
    })

    callback();
};
