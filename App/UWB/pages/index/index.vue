<template>
	<view class="content" :style="{ height:+ H_page + 'px'}" style="background-color: #FFFFFF;" >
		
		<view style="border-radius: 2px;border: 1rpx solid #007AFF;padding: 20rpx;margin: 20rpx;">按钮</view>
		
		<view class="welcome_text">欢迎使用<br />UWB定位系统</view>
		<button class="btn" @click="water(0)">开</button>
		<view @click="nav_sector()" :style="button" class="welcome_but">
			<view style="width: 80rpx;height: 80rpx;line-height: 80rpx;">进入</view>
		</view>
	</view>
</template>

<script>
	// ANCHOR1:0.0,0.0, ANCHOR2:512.0,0.0, ANCHOR3:0.0,1280.0
	// TAG1: 0.0, 0.0
	export default {
		data() {
			return {
				H_page: 0,
				H_but: 0,
				button: '',
				ts_s: 0,
				ts_e: 0,
				fee: 0,
				state: '就绪'
			}
		},
		methods: {
			nav_sector() {
				uni.reLaunch({
					url: '../sector/sector' + '?log=1'
				})
			},
			water(idx) {
				var date = new Date()
				var ts_s = this.ts_s
				var ts_e = this.ts_e
				if (idx == 0) {
					if (this.state == '就绪') {
						ts_s = date.getTime()
						this.state = '出水中'
						this.fee = 0
					} else if (this.state == '暂停出水') {
						ts_s = date.getTime()
						this.state = '出水中'
					}
				} else if (idx == 1) {
					if (this.state == '出水中') {
						ts_e = date.getTime()
						this.state = '暂停出水'
						this.fee += (ts_e - ts_s) / 1000
					}
				} else if (idx == 2) {
					ts_e = date.getTime()
					if (this.state == '暂停出水') {
						this.fee = this.fee
					} else if (this.state == '出水中') {
						if (this.fee == 0) {
							this.fee = (ts_e - ts_s) / 1000
						} else {
							this.fee += (ts_e - ts_s) / 1000
						}
					}
					ts_s = 0
					ts_e = 0
					this.state = '就绪'
				}
				this.ts_s = ts_s
				this.ts_e = ts_e
			}
		},
		onShow: function() {

		},
		onLoad: function() {
			uni.getSystemInfo({
				success: (res) => {
					this.H_page = res.windowHeight;
				}
			})
			this.H_but = this.H_page / 6;
			this.button = "margin-top:" + this.H_but + 'px';

		}

	}
</script>

<style>
	@import "../../common/index.css";
	.btn{
		background-color: #11FFBD;
		color: red;
		border-radius: 0;
	}
	.content {
		display: flex;
		flex-direction: column;
		justify-content: center;
		align-items: center;
		text-align: center;
		margin-top: auto;
		margin-bottom: auto;
	}

	.welcome_text {
		font-weight: 700;
		font-size: larger;
	}

	.welcome_but {
		border-radius: 100%;
		background: linear-gradient(145deg, #d9d9d9, #ffffff);
		box-shadow: 20px 20px 60px #cdcdcd,
			-20px -20px 60px #ffffff;
		padding: 50rpx;
	}
</style>
