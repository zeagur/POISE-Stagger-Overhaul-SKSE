#pragma once

class PoiseData {
protected:
	uint32_t oriPoise;
	uint32_t poise;
	float maxPoise;
	uint32_t lastStaggerTime;
	uint32_t nowTime;

public:
	// Default constructor
	PoiseData() : oriPoise(0), poise(0), maxPoise(0.0), lastStaggerTime(0), nowTime(0) {}

	// Parametrized constructor
	PoiseData(const uint32_t oriPoise, const uint32_t poise, const float maxPoise,
		const uint32_t lastStaggerTime, const uint32_t nowTime)
	 : oriPoise(oriPoise), poise(poise), maxPoise(maxPoise),
	   lastStaggerTime(lastStaggerTime), nowTime(nowTime) {}

	// Getter and setter methods
	[[nodiscard]] uint32_t getOriPoise() const { return oriPoise; }
	[[nodiscard]] uint32_t getPoise() const { return poise; }
	[[nodiscard]] float getMaxPoise() const { return maxPoise; }
	[[nodiscard]] uint32_t getLastStaggerTime() const { return lastStaggerTime; }
	[[nodiscard]] uint32_t getNowTime() const { return nowTime; }

	void setOriPoise(const uint32_t value) { oriPoise = value; }
	void setPoise(const uint32_t value) { poise = value; }
	void setMaxPoise(const float value) { maxPoise = value; }
	void setLastStaggerTime(const uint32_t value) { lastStaggerTime = value; }
	void setNowTime(const uint32_t value) { nowTime = value; }
};