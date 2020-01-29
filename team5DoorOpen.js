const AWS = require("aws-sdk");
const ddb = new AWS.DynamoDB.DocumentClient();

exports.handler = (event, context ,callback) => {
    //스마트 도어락 ESP 모듈에서 문을 열었다는 신호를 IoTCore로 보내면,
    //DynamoDB에 문 열린 시간(Timstamp)을 저장한다.(테이블명은 iot_project_Door)
    ddb.scan({TableName:"iot_project_Door"}, function(err,data){
        if(err){console.log(err)}
        else{
            console.log(data["Count"])
            const params = {
                TableName:"iot_project_Door",
                Item : {
                    idx: data["Count"]+1,
                    openTime: event.state.reported.openTime
                }
            }
            ddb.put(params,function(err,data){if(err){}})
        }
    });

};
