'use strict';

const Core = require('@alicloud/pop-core');

var client = new Core({
	accessKeyId: 'LTAI4G7tduj5XzDK897foHW4',
	accessKeySecret: '2WtE7kvP53CNmvLM4xU5yV0eBmayz8',
	endpoint: 'https://iot.cn-shanghai.aliyuncs.com',
	apiVersion: '2018-01-20'
});

var params = {
	"DeviceName": [
		"Anchor1"
	],
	"ProductKey": "a1lI6KdcEY7"
}

var requestOption = {
	method: 'POST'
};



exports.main = async (event, context) => {
	client.request('BatchQueryDeviceDetail', params, requestOption).then((result) => {
		console.log(result);
	}, (ex) => {
		console.log(ex);
	})
	//返回数据给客户端
	return event
};
