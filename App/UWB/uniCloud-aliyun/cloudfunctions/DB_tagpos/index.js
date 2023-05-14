'use strict';
exports.main = async (event, context) => {
	// Get all records in collection <TagPos>, following descend order
	const db = uniCloud.database()
	const collection = db.collection('TagPos')
	return collection.orderBy("Time", "desc").get().then(res => {
		console.log(res.data)
		return res.data
	})
};
