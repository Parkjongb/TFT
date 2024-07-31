/*
 * DHT11.h
 *
 *  Created on: Jul 4, 2024
 *      Author: pjb
 */


#ifndef INC_DHT11_H_
#define INC_DHT11_H_

#include "main.h"
#include <string.h>
#include <stdio.h>

//DHT11모듈을 연결한 디지털핀 지정 (PA10)
#define DHT11_PORT GPIOA
#define DHT11_PIN GPIO_PIN_10
TIM_HandleTypeDef htim1;
//----- us단위 대기 함수
void delay_us(uint16_t time) {
	__HAL_TIM_SET_COUNTER(&htim1, 0);
	while((__HAL_TIM_GET_COUNTER(&htim1))<time);
}

//----- 펄스 대기 함수
int wait_pulse(int state) {
	__HAL_TIM_SET_COUNTER(&htim1, 0);
	while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) != state) {  // 설정한 상태로 변할 때까지 대기
		if(__HAL_TIM_GET_COUNTER(&htim1) >= 100) {              // 100us 안에 신호 들어오지 않으면 타임아웃으로 간주
			return 0;
		}
	}
	return 1;
}
// 온도, 습도 저장하는 전역변수 선언
int Temperature = 0;
int Humidity = 0;

int dht11_read (void) {
	//----- Start Signal 전송
	// 포트를 출력으로 설정
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = DHT11_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);

	// Low 18ms, High 20us 펄스 생성
	HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, 0);
	delay_us(18000);
  HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, 1);
  delay_us(20);

	// 포트를 입력으로 설정
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);

	//----- DHT11 응답 확인
	delay_us(40);  // 40us 대기
	if(!(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN))) {   // DHT11 응답 체크(Low)
		delay_us(80);
		if(!(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN))) // 80us뒤 DHT11 High 응답 없으면 timeout으로 간주
			return -1;
	}
	if(wait_pulse(GPIO_PIN_RESET) == 0) // 데이터 전송 시작 대기
		return -1; // timeout

	//----- DHT11 데이터 읽기
	uint8_t out[5], i, j;
	for(i = 0; i < 5; i++) {               // 습도 정수자리, 습도 소수자리, 온도 정수자리, 온도 소수자리, 체크섬 순으로 읽음
		for(j = 0; j < 8; j++) {           // 하나의 데이터는 8비트로 구성되며, 최상위 비트부터 하나씩 읽기 시작함
			if(!wait_pulse(GPIO_PIN_SET))  // 데이터 전송 시작까지 대기
				return -1;

			delay_us(40);  // 40us 대기 후 High상태이면 1, Low상태이면 0 수신
			if(!(HAL_GPIO_ReadPin (DHT11_PORT, DHT11_PIN)))   // Low일 경우 0
				out[i] &= ~(1<<(7-j));
			else                                              // High일 경우 1
				out[i] |= (1<<(7-j));

			if(!wait_pulse(GPIO_PIN_RESET)) // 다음 데이터 전송 시작까지 대기
				return -1;
		}
	}

	//----- 체크섬 판별
	if(out[4] != (out[0] + out[1] + out[2] + out[3]))
		return -2;

	//----- 필요 데이터 분리
	Temperature = out[2];
	Humidity = out[0];

	return 1;
}

#endif /* INC_DHT11_H_ */
