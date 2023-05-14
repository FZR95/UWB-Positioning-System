'use strict';
const Core = require('@alicloud/pop-core');

var client = new Core({
	accessKeyId: 'LTAI4G7tduj5XzDK897foHW4',
	accessKeySecret: '2WtE7kvP53CNmvLM4xU5yV0eBmayz8',
	endpoint: 'https://iot.cn-shanghai.aliyuncs.com',
	apiVersion: '2018-01-20'
});

var params = {
	"PageSize": "100",
	"ProductKey": "a1lI6KdcEY7",
	"DeviceName": "Anchor1",
	"StartTime": "1616206549000",
	"EndTime": "1622894913904",
	"Asc": "0",
	"Identifier": "tagpos"
}

var requestOption = {
	method: 'POST'
};


exports.main = async (event, context) => {
	var date = new Date();
	let curtime = date.getTime()
	
	params.StartTime = curtime - (6 * 60 * 60 * 1000)
	params.EndTime = curtime

	return client.request('QueryDevicePropertyData', params, requestOption).then((result) => {
		let listarr = result.Data.List.PropertyInfo
		const db = uniCloud.database()
		const collection = db.collection('TagPos')
		// Jump collection.add if the record is already exists, judged by Timestamp
		for (let i = 0; i < listarr.length; i++) {
			collection.where({
				Time: listarr[i].Time
			}).get().then(result => {
				if (!result.data.length)
					collection.add(listarr[i])
			})
		}
		return ("IOT_QueryDevicePropertyData_tagpos Success.");
	}, (ex) => {
		console.log("IOT_QueryDevicePropertyData_tagpos ERROR!")
		return "Err!"
	})
};
