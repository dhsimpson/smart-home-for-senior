const AWS = require("aws-sdk");
const ddb = new AWS.DynamoDB.DocumentClient();



// 안드로이드 App에서 한달 간 사회복지사가 출입한 횟수를 알고 싶을 때,
// 안드로이드로 부터 timestamp를 받아 iot_project_Door DB 테이블에서 최근 한 달간의 문 열기 횟수를 카운트 한다.

exports.handler = (event,context,callback) => {
    var timeStamp = event["timeStamp"];
    var cnt=0;

    timeStamp -= 2592000;//unix Time 기준 1달은 2592000 ms
    var params = {
            TableName:"iot_project_Door",
            ProjectionExpression: "openTime",
            FilterExpression: "#date >= :TimeStamp", //unix timestamp 로 1달 지난 놈들에 대해 query
            ExpressionAttributeNames:{
                "#date": "openTime",
            },
            ExpressionAttributeValues:{
                ":TimeStamp":timeStamp
            }
        };
        ddb.scan(params, function(err, data){
            if(err){
                console.log(err);
            }else{
                var cnt = data["Count"];
                context.succeed({"enterCount" : cnt})
            }
        });
};
