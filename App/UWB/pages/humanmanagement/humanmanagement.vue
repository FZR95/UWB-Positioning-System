<template>
	<view>
		<view v-for="(HUM, index) in human" :key="index">
			<view class="list" :class="{emergent:HUM.emergent}">
				<view class="header">
					<view>姓名：{{HUM.name}}</view>
					<view>状态：{{HUM.stat}}</view>
					<svg height="20px" width="20px">
						<circle cx="10" cy="10" r="8" fill="#F0AD4E" v-if="HUM.sigstr==='弱'" />
						<circle cx="10" cy="10" r="8" fill="#2ecc71" v-else-if="HUM.stat==='在岗'" />
						<circle cx="10" cy="10" r="8" fill="#e74c3c" v-else />
					</svg>
				</view>
				<view class="info" v-if="HUM.stat==='在岗'">
					<view>到达时间：{{HUM.arrt}}</view>
					<view>所在区域：{{HUM.pos}}</view>
					<view>信号状态：{{HUM.sigstr}}</view>
					<view v-if="HUM.late" style="text-align: right;">-迟到</view>
					<view v-if="HUM.emergent" style="text-align: right;">-{{HUM.report}}</view>
					<view v-if="HUM.emergent" style="text-align: right;">-已开启增强定位</view>
				</view>
			</view>

		</view>
	</view>
</template>

<script>
	export default {
		data() {
			return {
				human: [{
					name: '何晓牛',
					arrt: '2021-6-5 07:52',
					pos: '区域B',
					stat: '在岗',
					late: false,
					emergent: false
				},
				{
					name: '董立冰',
					arrt: '',
					pos: '区域B',
					stat: '未打卡',
					late: false,
					emergent: false
				},
				{
					name: '张杰瑞',
					arrt: '2021-6-5 07:59',
					pos: '区域C',
					stat: '在岗',
					late: false,
					emergent: true,
					report:'已报警',
					sigstr:'弱'
				},
				{
					name: '刘晴微',
					arrt: '2021-6-5 08:01',
					pos: '区域A',
					stat: '在岗',
					late: true,
					emergent: false
				}]
			}
		},
		methods: {

		}
	}
</script>

<style>
	@import "../../common/human.css";
</style>
