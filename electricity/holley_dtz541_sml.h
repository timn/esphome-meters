#include "esphome.h"

#include <sstream>
#include <string>
#include <variant>
#include <vector>

static const char kkWh[] = "kWh";
static const char kWatt[] = "W";
static const char kVolt[] = "V";
static const char kAmpere[] = "A";
static const char kHz[] = "Hz";

static const char kEnergy[] = "energy";
static const char kVoltage[] = "voltage";
static const char kCurrent[] = "current";
static const char kFrequency[] = "frequency";

static const char kEnergyIn[] = "Energy In";
static const char kEnergyOut[] = "Energy Out";
static const char kActivePower[] = "Active Power";
static const char kVoltageL1[] = "Voltage L1";
static const char kVoltageL2[] = "Voltage L2";
static const char kVoltageL3[] = "Voltage L3";
static const char kAmperageL1[] = "Amperage L1";
static const char kAmperageL2[] = "Amperage L2";
static const char kAmperageL3[] = "Amperage L3";
static const char kFrequencySensor[] = "Frequency";
static const char kInvalidData[] = "Invalid Data";

static const char kEnergyInObjId[] = "energy_in";
static const char kEnergyOutObjId[] = "energy_out";
static const char kActivePowerObjId[] = "active_power";
static const char kVoltageL1ObjId[] = "voltage_l1";
static const char kVoltageL2ObjId[] = "voltage_l2";
static const char kVoltageL3ObjId[] = "voltage_l3";
static const char kAmperageL1ObjId[] = "amperage_l1";
static const char kAmperageL2ObjId[] = "amperage_l2";
static const char kAmperageL3ObjId[] = "amperage_l3";
static const char kFrequencySensorObjId[] = "frequency";
static const char kInvalidDataObjId[] = "invalid_data";

uint8_t *SkipField(uint8_t *cp) {
	uint8_t len = *cp & 0xf;
	uint8_t type = *cp & 0x70;
	if (type == 0x70) { // list, skip entries
		cp++;
		while (len--) {
			uint8_t len1 = *cp&0x0f;
			cp += len1;
		}
	} else {
		// skip len
		cp += len;
	}
	return cp;
}

std::string StringToHex(const std::string& s) {
	std::string rv;
	for (size_t i = 0; i < s.size() - 1; i+= 2) {
		std::string t = s.substr(i, 2);
		size_t pos;
		rv.push_back(stoi(t, &pos, /*base=*/16));
	}
	return rv;
}

/*
std::string HexToString(std::string_view h) {
	std::string rv;
	static const char hex_chars[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
		'A', 'B', 'C', 'D', 'E', 'F'};
	rv.reserve(h.size() * 2);
	for (size_t i = 0; i < h.size(); ++i) {
		rv.push_back(hex_chars[(h[i] & 0xf0) >> 4]);
		rv.push_back(hex_chars[h[i] & 0x0f]);
	}
	return rv;
}
*/

/*
void PrintChunked(std::string_view message) {
	if (message.size() < 200) {
		ESP_LOGI("custom", "Message: %s\n", HexToString(message).c_str());
		return;
	}
	int i = 0;
	for (i = 0; i < message.size() - 200; i += 200) {
		ESP_LOGI("custom", "Message %d-%d: %s\n", i, i+200, HexToString(message.substr(i, i+200)).c_str());
	}
	if (i < message.size()) {
		ESP_LOGI("custom", "Message %d-%d: %s\n", i, message.size(), HexToString(message.substr(i)).c_str());			
	}
}
*/

class SmlValue final {
 public:

	template <typename T>
	SmlValue(T value) : value_(value) {}
	SmlValue() = default;

	template<typename T>
	T GetAs() const {
		if (! std::holds_alternative<T>(value_)) {
			return 0;
		}
		return std::get<T>(value_);
	}

	std::string GetAs() const {
		if (! std::holds_alternative<std::string>(value_)) {
			return "";
		}
		return std::get<std::string>(value_);
	}

	template<typename T>
	bool Is() const {
		return std::holds_alternative<T>(value_);
	}

	double AsDouble() {
		if (Is<int8_t>()) {
			return GetAs<int8_t>() * std::pow(10, scaler_);
		} else if (Is<int16_t>()) {
			return GetAs<int16_t>() * std::pow(10, scaler_);
		} else if (Is<int32_t>()) {
			return GetAs<int16_t>() * std::pow(10, scaler_);
		} else if (Is<int64_t>()) {
			return GetAs<int16_t>() * std::pow(10, scaler_);
		} else if (Is<uint8_t>()) {
			return GetAs<uint8_t>() * std::pow(10, scaler_);
		} else if (Is<uint16_t>()) {
			return GetAs<uint16_t>() * std::pow(10, scaler_);
		} else if (Is<uint32_t>()) {
			return GetAs<uint32_t>() * std::pow(10, scaler_);
		} else if (Is<uint64_t>()) {
			return GetAs<uint64_t>() * std::pow(10, scaler_);
		} else {
			return std::numeric_limits<double>::quiet_NaN();
		}
	}

	bool IsValid() const {
		return (! std::holds_alternative<std::monostate>(value_));
	}

	void Invalidate() {
		value_ = std::monostate();
	}

	void SetDoubleFactor(double factor) {
		double_factor_ = factor;
	}

	void SetScaler(int8_t scaler) {
		scaler_ = scaler;
	}

private:
	using VariantType = std::variant<std::monostate,
	                                 int8_t, int16_t, int32_t, int64_t,
	                                 uint8_t, uint16_t, uint32_t, uint64_t,
	                                 bool, std::string>;
	VariantType value_;
	double double_factor_ = 1.;
	int8_t scaler_ = 0.;
};


SmlValue ReadField(const uint8_t* data) {
	uint8_t type_len = *data & 0x7f;
	uint8_t type = type_len & 0x70;
	uint8_t len = type_len & 0x0f;

	data += 1;

	if (type_len == 0x01) {
		// omitted optional value
		return SmlValue();
	}

	switch (type) {
	case 0x00: // octet string
		return SmlValue(std::string(reinterpret_cast<const std::string::value_type*>(data), len));
	case 0x40: // bool
		return SmlValue((*data != 0x00));
	case 0x50: // signed int
		[[fallthrough]];
	case 0x60: // unsigned int
		{
			uint64_t uvalue = 0;
			for (int i = 0; i < len-1; ++i) {
 				uvalue <<= 8;
				uvalue |= *data++;
			}

			if (type == 0x60) { // unsigned
				switch (len-1) {
				case 1:
					return SmlValue(static_cast<uint8_t>(uvalue));
				case 2:
					return SmlValue(static_cast<uint16_t>(uvalue));
				case 4:
					return SmlValue(static_cast<uint32_t>(uvalue));
				case 8:
					return SmlValue(uvalue);
				default:
					return SmlValue();
				}
			}
			if (type == 0x50) { // signed
				switch (len-1) {
				case 1:
					return SmlValue(static_cast<int8_t>(uvalue));
				case 2:
					return SmlValue(static_cast<int16_t>(uvalue));
				case 4:
					return SmlValue(static_cast<int32_t>(uvalue));
				case 8:
					return SmlValue(static_cast<int64_t>(uvalue));
				default:
					return SmlValue();
				}
			}
		}
	}
	return SmlValue();
}

struct ValueSpec {
	using ValueFunction = std::function<void(const SmlValue&, SmlValue&)>;

	ValueSpec(const std::string& hex_pattern, int8_t scaler, Sensor* sensor,
	          std::optional<float> max_expected_change = std::nullopt,
	          std::optional<ValueFunction> value_function = std::nullopt)
	: pattern(StringToHex(hex_pattern)), scaler(scaler),
	  max_expected_change(max_expected_change), sensor(sensor),
	  value_function(value_function)
	{
		if (max_expected_change.has_value()) {
			// The amount of consistent values we need before considering the sensor initialized
			init_values.resize(3);
		}
	}

	// Spec
	std::string pattern;
	int8_t scaler;
	std::optional<float> max_expected_change;

	// result
	SmlValue current_value;
  Sensor* sensor;
	SmlValue last_value;
	std::vector<SmlValue> init_values;
	int init_values_next_index = 0;

	std::optional<ValueFunction> value_function;
};

class HolleyDtz541SmlComponent : public Component, public UARTDevice {
 public:
	enum PowerDirection { kIn, kOut };

	HolleyDtz541SmlComponent(UARTComponent *parent) : UARTDevice(parent),
	                                                  end_marker_(StringToHex("1b1b1b1b1a")),
	                                                  end_marker_view_(end_marker_) {
 
		// Would need exposing a text sensor
		// {"77070100600100ff", "server_id" },
	  specs_ = {
		  {"77070100010800ff", /*scaler=*/-3,
		   CreateSensor(kEnergyIn, kEnergyInObjId, kkWh, kEnergy, STATE_CLASS_TOTAL_INCREASING, 4),
		   /*max_expected_change=*/0.005, // 18 kWh
		   [this](const SmlValue& status, SmlValue& value) {
			   if (status.Is<uint64_t>()) {
				   if (status.GetAs<uint64_t>() & 0x00000800) {
					   power_direction_ = kOut;
				   } else {
					   power_direction_ = kIn;
				   }
			   }
		   }},
		  {"77070100020800ff", /*scaler=*/-3,
		   CreateSensor(kEnergyOut, kEnergyOutObjId, kkWh, kEnergy, STATE_CLASS_TOTAL_INCREASING, 4),
		   /*max_expected_change=*/0.005}, // 18 kWh
		  {"77070100100700ff", 0,
		   CreateSensor(kActivePower, kActivePowerObjId, kWatt, "power", STATE_CLASS_MEASUREMENT, 0),
		   /*max_expected_change=*/5000.,
		   [this](const SmlValue& status, SmlValue& value) {
			   if (power_direction_ == kOut) {
				   value.SetDoubleFactor(-1.);
			   }
		   }},
		  {"77070100340700ff", /*scaler=*/0,
		   CreateSensor(kVoltageL1, kVoltageL1ObjId, kVolt, kVoltage, STATE_CLASS_MEASUREMENT, 2)},
		  {"77070100340700ff", /*scaler=*/0,
		   CreateSensor(kVoltageL2, kVoltageL2ObjId, kVolt, kVoltage, STATE_CLASS_MEASUREMENT, 2)},
		  {"77070100480700ff", /*scaler=*/0,
		   CreateSensor(kVoltageL3, kVoltageL3ObjId, kVolt, kVoltage, STATE_CLASS_MEASUREMENT, 2)},
		  {"770701001f0700ff", /*scaler=*/0,
		   CreateSensor(kAmperageL1, kAmperageL1ObjId, kAmpere, kCurrent, STATE_CLASS_MEASUREMENT, 2)},
		  {"77070100330700ff", /*scaler=*/0,
		   CreateSensor(kAmperageL2, kAmperageL2ObjId, kAmpere, kCurrent, STATE_CLASS_MEASUREMENT, 2)},
		  {"77070100470700ff", /*scaler=*/0,
		   CreateSensor(kAmperageL3, kAmperageL3ObjId, kAmpere, kCurrent, STATE_CLASS_MEASUREMENT, 2)},
		  {"770701000e0700ff", /*scaler=*/0,
		   CreateSensor(kFrequencySensor, kFrequencySensorObjId, kHz, kFrequency, STATE_CLASS_MEASUREMENT, 1)}
	  };

	  invalid_data_sensor_ = CreateSensor(kInvalidData, kInvalidDataObjId, "", "", STATE_CLASS_TOTAL, 0);
  }

  void setup() override {
  }

	void loop() override {
		invalid_data_counter_ = 0;

		while (available()) {
      char c = read();
      buffer_.put(c);
      // C++ 20 only
      //std::string_view v = buffer_.view();
      std::string s = buffer_.str();
      std::string_view v(s);
      std::string::size_type end_index = 0;
      if ((end_index = v.find(end_marker_view_)) != std::string::npos) {
	      end_index += end_marker_view_.length();
	      std::string_view message = v.substr(0, end_index);
	      buffer_.str(std::string(s.substr(message.size())));
	      ExtractData(message);
        ProcessData();
        PublishData();
      }
      if (s.size() > 1000) {
	      ESP_LOGI("custom", "Clearing buffer");
	      // something went wrong, reset buffer
	      buffer_.str(std::string());
      }
    }
  }

	std::vector<Sensor *> GetSensors() const {
		std::vector<Sensor *> sensors;
		sensors.reserve(specs_.size());
		for (const ValueSpec& value : specs_) {
			sensors.push_back(value.sensor);
		}
		return sensors;
	}

 private:
	void ExtractData(std::string_view input) {
		for (ValueSpec& value : specs_) {
			std::string::size_type idx;
			if ((idx = input.find(value.pattern)) != std::string::npos) {
				const uint8_t* data = reinterpret_cast<const uint8_t*>(input.data() + idx + value.pattern.size());
				uint8_t* cp = const_cast<uint8_t*>(data);
				// status
				SmlValue status = ReadField(cp);
				cp = SkipField(cp);
				// time
				cp=SkipField(cp);
				// unit
				cp=SkipField(cp);
				SmlValue scaler_field = ReadField(cp);
				cp=SkipField(cp);

				value.current_value = ReadField(cp);
				int8_t scaler = 0;
				if (scaler_field.Is<int8_t>()) {
					scaler = scaler_field.GetAs<int8_t>();
				}
				value.current_value.SetScaler(scaler + value.scaler);
			}
		}
	}

	bool InitValuesConsistent(ValueSpec& value) {
		if (!value.max_expected_change.has_value()) {
			return true;
		}
		if (value.init_values_next_index == value.init_values.size()) {
			return true;
		}

		value.init_values[value.init_values_next_index] = value.current_value;
		value.init_values_next_index += 1;

		for (int i = 1; i < value.init_values_next_index; ++i) {
			if (fabs(value.init_values[i].AsDouble() - value.init_values[i-1].AsDouble()) > *value.max_expected_change) {
				value.init_values_next_index = 0;
				return false;
			}
		}

		// Values are consistent if we made it here and have collected expected
		// number of values
		return (value.init_values_next_index == value.init_values.size());
	}

	void ProcessData() {
		for (ValueSpec& value : specs_) {
			// There is no criterion to validate, assume data is ok.
			if (!value.max_expected_change.has_value()) continue;

			if (!value.last_value.IsValid()) {
				if (InitValuesConsistent(value)) {
					value.last_value = value.current_value;
				} else {
				  ESP_LOGI("custom", "Value %s is not yet consistent", value.sensor->get_name().c_str());
				  invalid_data_counter_ += 1;
					value.current_value.Invalidate();
				}
				continue;
			}

			if (value.sensor->get_state_class() == STATE_CLASS_TOTAL_INCREASING &&
			    value.current_value.AsDouble() < value.current_value.AsDouble()) {
				ESP_LOGI("custom", "Value %s is expected to only increase, but current %f < last %f",
				         value.sensor->get_name().c_str(),
				         value.current_value.AsDouble(), value.last_value.AsDouble());
				invalid_data_counter_ += 1;
				value.last_value.Invalidate();
				value.init_values_next_index = 0;
				value.current_value.Invalidate();
				continue;
			}
			if (value.last_value.AsDouble() != 0.0 &&
			    fabs(value.last_value.AsDouble() - value.current_value.AsDouble()) > *value.max_expected_change) {
				ESP_LOGI("custom", "Value %s exceeds expected change(fabs(%f - %f) > %f",
				         value.sensor->get_name().c_str(),
				         value.last_value.AsDouble(), value.current_value.AsDouble(), *value.max_expected_change);
				invalid_data_counter_ += 1;
				value.last_value.Invalidate();
				value.init_values_next_index = 0;
				value.current_value.Invalidate();
				continue;
			}
			// Current value is acceptable, store as last value
			value.last_value = value.current_value;
		}
	}

	void PublishData() {
		for (ValueSpec& value : specs_) {
			if (value.current_value.IsValid()) {
				value.sensor->publish_state(value.current_value.AsDouble());
			}
		}
		invalid_data_sensor_->publish_state(invalid_data_counter_);
	}

	// name is stored as a reference and must exits for as long as the sensor does.
	Sensor* CreateSensor(const char* name,
	                     const char* object_id,
	                     const char* unit,
	                     const char* device_class,
	                     StateClass state_class,
	                     int8_t accuracy_decimals) {
		Sensor* sensor = new Sensor();
		sensor->set_name(name);
		// Required (and possible) only with ESPHome 2023.4.0 and later
		sensor->set_object_id(object_id);
		sensor->set_unit_of_measurement(unit);
		sensor->set_device_class(device_class);
		sensor->set_state_class(state_class);
		sensor->set_accuracy_decimals(accuracy_decimals);
		App.register_sensor(sensor);
		return sensor;
	}

 private:
	std::vector<ValueSpec> specs_;
	std::stringstream buffer_;
  std::string end_marker_;
	std::string_view end_marker_view_;
	PowerDirection power_direction_ = kIn;
	int invalid_data_counter_ = 0;
	Sensor* invalid_data_sensor_;
};

