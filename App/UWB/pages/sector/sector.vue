<template>
	<view class="content">
		<view v-for="(SEC, index) in sections" :key="index">
			<view class="header sector" :class="{emergent:SEC.emergent}">
				<view class="header_text" @click="IOT_init()">
					{{SEC.name}}
				</view>
				<view class="header_status">
					<svg height="20px" width="20px">
						<circle cx="10" cy="10" r="8" fill="#2ecc71" v-if=online />
						<circle cx="10" cy="10" r="8" fill="#e74c3c" v-else />
					</svg>
					{{status}}
					<image src="../../static/refresh.png" style="width: 30rpx;height: 30rpx;margin-left: 20rpx;">
					</image>
				</view>
				<view @click="SEC.is_sector_fold = !SEC.is_sector_fold">
					<image src="../../static/fold.png" v-if="!SEC.is_sector_fold"></image>
					<image src="../../static/fold.png" class="reversed" v-if="SEC.is_sector_fold"></image>
				</view>
			</view>
			<view v-show="SEC.is_sector_fold">
				<view class="uwbmap" v-if="online">
					<view class="sector_title optheader">
						<view>地图 </view>
						<view class="opt">
							实时更新
							<switch :checked="uwbmap_update" color="#2ecc71" @change="uwbmap_switch_update" />
							显示轨迹
							<switch :checked="uwbmap_track" color="#2ecc71" @change="uwbmap_switch_track" />
						</view>
					</view>
					<canvas canvas-id="uwbmap" id="uwbmap" :style="uwbmap_style"></canvas>
					<view class="map_info">
						<view class="info">
							<view class="info_header"> 标签坐标(cm) </view>
							<view class="info_content">
								<view v-for="(item,index) in tagpos" :key="index">
									{{item.name}}:{{item.x}}, {{item.y}}
								</view>
							</view>
						</view>
						<view class="info">
							<view class="info_header"> 地图信息 </view>
							<view class="info_content">
								地图宽度：{{mapinfo.width/100}}m <br />
								地图高度：{{mapinfo.height/100}}m
							</view>
						</view>
						<view class="info">
							<view class="info_header"> 基站坐标(cm) </view>
							<view class="info_content">
								<view v-for="(item,index) in anchorpos" :key="index">
									{{item.name}}:{{item.x}}, {{item.y}}, {{item.z}}
								</view>
							</view>
						</view>
					</view>
				</view>
				<view class="information sector" v-if="online">
					<view class="sector_title"> 设备属性 </view>
					<view v-for="(item,index) in information" :key="index" class="info"
						@click="go_log(item.Identifier)">
						<view class="info_header">
							属性名：{{item.Identifier}}
						</view>
						<view class="info_content">
							属性值：{{item.Value}}
						</view>
						<view class="info_time">
							上传时间：{{item.Time}}
						</view>
					</view>
				</view>
			</view>
		</view>
	</view>
</template>

<script>
	// ANCHOR1:0.0,0.0, ANCHOR2:512.0,0.0, ANCHOR3:0.0,1280.0
	// TAG1: 0.0, 0.0
	export default {
		data() {
			return {
				sections: [{
					name: '区域A',
					is_sector_fold: true,
					status: 'OFFLINE',
					emergent:false
				}, {
					name: '区域B',
					is_sector_fold: true,
					status: 'OFFLINE',
					emergent:false
				}, {
					name: '区域C',
					is_sector_fold: true,
					status: 'OFFLINE',
					emergent:true
				}],
				status: 'OFFLINE',
				online: false,
				uwbmap_style: '',
				uwbmap_update: false,
				uwbmap_track: true,
				uwbmap_interval: '',
				information: {},
				mapinfo: {},
				tagpos: [],
				anchorpos: []
			}
		},
		methods: {
			sector_fold(dat) {
				this.is_sector_fold[dat] = !this.is_sector_fold[dat];
			},
			canvas_map_draw() {
				let obj = {
					anchor_num: 3,
					tag_num: 4,
					anchor_font_size: 15,
					anchor_radius: 15,
					anchor_stroke: 5,
					tag_font_size: 8,
					tag_radius: 8,
					tag_stroke: 5
				}
				var context = uni.createCanvasContext('uwbmap')
				// Draw Border
				context.setStrokeStyle('black')
				context.setLineWidth(1)
				context.rect(
					this.anchorpos[0].x * this.mapinfo.ratio_width + obj.anchor_radius + obj.anchor_stroke,
					this.anchorpos[0].y * this.mapinfo.ratio_height + obj.anchor_radius + obj.anchor_stroke,
					this.mapinfo.canvas_width,
					this.mapinfo.canvas_height)
				context.stroke()
				context.draw(this.uwbmap_track)
				// Draw Anchor
				for (let i = 0; i < obj.anchor_num; i++) {
					context.setFillStyle('#f1c40f')
					context.setStrokeStyle('black')
					context.setLineWidth(obj.anchor_stroke - 4)
					context.arc(
						this.anchorpos[i].x * this.mapinfo.ratio_width + obj.anchor_radius + obj.anchor_stroke,
						this.anchorpos[i].y * this.mapinfo.ratio_height + obj.anchor_radius + obj.anchor_stroke,
						obj.anchor_radius, 0,
						Math.PI * 2)
					context.fill()
					context.stroke()
					context.draw(true)
					context.setFillStyle('black')
					context.setFontSize(obj.anchor_font_size)
					context.fillText(
						this.anchorpos[i].name,
						this.anchorpos[i].x * this.mapinfo.ratio_width + obj.anchor_radius + obj.anchor_stroke - obj
						.anchor_font_size / 1.5,
						this.anchorpos[i].y * this.mapinfo.ratio_height + obj.anchor_radius + obj.anchor_stroke + obj
						.anchor_font_size / 3)
					context.draw(true)
				}
				// Draw Tag
				for (let i = 0; i < obj.tag_num; i++) {
					context.setFillStyle('#2ecc71')
					context.arc(
						this.tagpos[i].x * this.mapinfo.ratio_width + obj.tag_radius + obj.tag_stroke,
						this.tagpos[i].y * this.mapinfo.ratio_height + obj.tag_radius + obj.tag_stroke,
						obj.tag_radius, 0,
						Math.PI * 2)
					context.fill()
					context.draw(true)
					context.setFillStyle('black')
					context.setFontSize(obj.tag_font_size)
					context.fillText(
						this.tagpos[i].name,
						this.tagpos[i].x * this.mapinfo.ratio_width + +obj.tag_radius + obj.tag_stroke - obj
						.tag_font_size / 1.5,
						this.tagpos[i].y * this.mapinfo.ratio_height + obj.tag_radius + obj.tag_stroke + obj
						.tag_font_size / 3)
					context.draw(true)
				}
			},
			canvas_map() {
				let win_w, win_h;
				uni.getSystemInfo({
					success: (res) => {
						win_w = res.windowWidth - 20;
						win_h = res.windowHeight * 0.6;
					}
				})
				this.uwbmap_style = "width:" + win_w + "px;" + "height:" + win_h +
					"px;";
				this.mapinfo.canvas_width = win_w * 0.85;
				this.mapinfo.canvas_height = win_h * 0.85;
				this.mapinfo.ratio_width = this.mapinfo.canvas_width / this.mapinfo.width;
				this.mapinfo.ratio_height = this.mapinfo.canvas_height / this.mapinfo.height;
				this.canvas_map_draw();
			},
			IOT_init() {
				// this.uwbmap_interval = setInterval(this.IOT_check_status(), 1000);
				this.IOT_check_status();
			},
			IOT_check_status() {
				let that = this;
				uniCloud.callFunction({
					name: 'IOT_GetDeviceStatus',
					success: (res) => {
						that.status = res.result;
						this.online = (this.status == "ONLINE")
						this.IOT_get_property()
					}
				})
			},
			IOT_get_property() {
				let that = this;
				// Fetch device property
				uniCloud.callFunction({
					name: 'IOT_QueryDevicePropertyStatus',
					success: (res) => {
						console.log(res)
						let arr = res.result;
						let anchor_info, tag_info;
						// Sort Anchor or Tag
						for (let i = 0; i < arr.length; i++) {
							arr[i].Time = that.timestampToTime(arr[i].Time)
							if (arr[i].Identifier == "anchorpos")
								anchor_info = arr[i].Value.split(';')
							if (arr[i].Identifier == "tagpos")
								tag_info = arr[i].Value.split(';')
						}
						// Provide Raw information
						that.information = arr;
						// Handle Anchor info
						for (let i = 0; i < anchor_info.length; i++) {
							let obj = {};
							let strarr = anchor_info[i].split(':');
							obj.name = strarr[0]
							obj.x = strarr[1].split(',')[0]
							obj.y = strarr[1].split(',')[1]
							obj.z = strarr[1].split(',')[2]
							that.anchorpos[i] = obj;
						}
						// Handle Tag info
						for (let i = 0; i < tag_info.length; i++) {
							let obj = {};
							let strarr = tag_info[i].split(':')
							obj.name = strarr[0]
							obj.x = strarr[1].split(',')[0]
							obj.y = strarr[1].split(',')[1]
							that.tagpos[i] = obj;
						}
						// TODO: Changes needed if Anchor positions uncertain
						that.mapinfo.width = (that.anchorpos[1].x - that.anchorpos[0].x);
						that.mapinfo.height = (that.anchorpos[2].y - that.anchorpos[0].y);
						that.canvas_map()
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
			uwbmap_switch_track(e) {
				this.uwbmap_track = !this.uwbmap_track

			},
			uwbmap_switch_update(e) {
				this.uwbmap_update = !this.uwbmap_update
				if (!this.uwbmap_update)
					clearInterval(this.uwbmap_interval)
				else
					this.uwbmap_interval = setInterval(this.IOT_check_status, 1000)
			},
			go_log(e) {
				if (e == "tagpos")
					uni.navigateTo({
						url: '../log/log'
					})
			}
		},
		onShow: function() {
			this.IOT_init()
		},
		onLoad: function(e) {
			this.is_sector_fold[0] = false;
			this.is_sector_fold[1] = false;
			this.is_sector_fold[2] = false;
			if (!e.log)
				uni.navigateTo({
					url: '../index/index'
				})
		}

	}
</script>

<style>
	@import "../../common/index.css";
</style>
