'use strict';
const Core = require('@alicloud/pop-core');

var client = new Core({
	accessKeyId: 'LTAI4G7tduj5XzDK897foHW4',
	accessKeySecret: '2WtE7kvP53CNmvLM4xU5yV0eBmayz8',
	endpoint: 'https://iot.cn-shanghai.aliyuncs.com',
	apiVersion: '2018-01-20'
});

var params = {
	"ProductKey": "a1lI6KdcEY7",
	"DeviceName": "Anchor1"
} 

var requestOption = {
	method: 'POST'
};

exports.main = async (event, context) => {
	return client.request('GetDeviceStatus', params, requestOption).then((result) => {
		return result.Data.Status
	}, (ex) => {
		console.log(ex);
		return "Err!"
	})
};
