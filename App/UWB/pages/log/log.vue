<template>
	<view class="main">
		<view class="information sector">
			<view class="sector_title">日志信息 - Tagpos</view>
			<view v-for="(item,index) in taglog" :key="index" class="info">
				<view class="info_content">
					属性值：{{item.Value}}
				</view>
				<view class="info_time">
					上传时间：{{item.Time}}
				</view>
			</view>
		</view>
	</view>
</template>

<script>
	export default {
		data() {
			return {
				taglog: []
			}
		},
		methods: {
			get_taglog() {
				uniCloud.callFunction({
					name: 'IOT_tagpos'
				})
				let that = this
				uniCloud.callFunction({
					name: 'DB_tagpos',
					success: (res) => {
						let dat = res.result
						that.taglog = dat
						for (let i = 0; i < dat.length; i++) {
							that.taglog[i].Time = that.timestampToTime(that.taglog[i].Time)
						}
					}
				})
			},
			timestampToTime(timestamp) {
				var date = new Date(timestamp * 1);
				var Y = date.getFullYear() + '-';
				var M = (date.getMonth() + 1 < 10 ? '0' + (date.getMonth() + 1) : date.getMonth() + 1) + '-';
				var D = (date.getDate() < 10 ? ('0' + date.getDate()) : date.getDate()) + ' ';
				var h = (date.getHours() < 10 ? ('0' + date.getHours()) : date.getHours()) + ':';
				var m = (date.getMinutes() < 10 ? ('0' + date.getMinutes()) : date.getMinutes()) + ':';
				var s = date.getSeconds();
				return Y + M + D + h + m + s;
			},
		},
		onLoad() {
			
		},
		onShow(){
			this.get_taglog()
		}
	}
</script>

<style>
	@import "../../common/index.css";

	.main {
		margin: 0 20rpx;
		padding-bottom: 20rpx;
	}
</style>
