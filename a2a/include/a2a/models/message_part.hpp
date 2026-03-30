/*
 * @Author: eren dengdengd1222@mail.com
 * @Date: 2026-03-30 14:52:03
 * @LastEditors: eren dengdengd1222@mail.com
 * @LastEditTime: 2026-03-30 15:11:39
 * @FilePath: /my_agent_communication/a2a/include/a2a/models/message_part.hpp
 * @Description: 
 * 
 */
#pragma once

#include "../core/types.hpp"
#include <string>
#include <memory>
#include <vector>

namespace a2a {

    /**
 * @brief Base class for message parts (polymorphic)
 */

class Part {
public:
    virtual ~Part() = default;
    
    virtual PartKind kind() const = 0;
    virtual std::string to_json() const = 0;
    virtual std::unique_ptr<Part> clone() const = 0;
    
    static std::unique_ptr<Part> from_json(const std::string& json);
};

/**
 * @brief Text message part
 */
class TextPart : public Part {
public:
    TextPart() = default;
    explicit TextPart(const std::string& text) : text_(text) {}
    
    PartKind kind() const override { return PartKind::Text; }
    
    const std::string& text() const { return text_; }
    void set_text(const std::string& text) { text_ = text; }
    
    std::string to_json() const override;
    std::unique_ptr<Part> clone() const override {
        return std::make_unique<TextPart>(text_);
    }

private:
    std::string text_;
};

/**
 * @brief File message part
 */
class FilePart : public Part {
public:
    FilePart() = default;
    FilePart(const std::string& filename, const std::string& mime_type, 
             const std::vector<uint8_t>& data)
        : filename_(filename)
        , mime_type_(mime_type)
        , data_(data) {}
    
    PartKind kind() const override { return PartKind::File; }
    
    const std::string& filename() const { return filename_; }
    const std::string& mime_type() const { return mime_type_; }
    const std::vector<uint8_t>& data() const { return data_; }
    
    void set_filename(const std::string& name) { filename_ = name; }
    void set_mime_type(const std::string& type) { mime_type_ = type; }
    void set_data(const std::vector<uint8_t>& data) { data_ = data; }
    
    std::string to_json() const override;
    std::unique_ptr<Part> clone() const override {
        return std::make_unique<FilePart>(filename_, mime_type_, data_);
    }

private:
    std::string filename_;
    std::string mime_type_;
    std::vector<uint8_t> data_;
};

/**
 * @brief Data message part (structured data)
 */
class DataPart : public Part {
public:
    DataPart() = default;
    explicit DataPart(const std::string& data_json) : data_json_(data_json) {}
    
    PartKind kind() const override { return PartKind::Data; }
    
    const std::string& data_json() const { return data_json_; }
    void set_data_json(const std::string& json) { data_json_ = json; }
    
    std::string to_json() const override;
    std::unique_ptr<Part> clone() const override {
        return std::make_unique<DataPart>(data_json_);
    }

private:
    std::string data_json_;
};

// Part factory method
std::unique_ptr<Part> Part::from_json(const std::string& json) {
    // Simplified parsing - in production use nlohmann/json
    
    // Determine kind
    size_t kind_pos = json.find("\"kind\":");
    if (kind_pos == std::string::npos) {
        return nullptr;
    }
    
    size_t kind_start = json.find("\"", kind_pos + 7) + 1;
    size_t kind_end = json.find("\"", kind_start);
    std::string kind = json.substr(kind_start, kind_end - kind_start);
    
    if (kind == "text") {
        size_t text_pos = json.find("\"text\":");
        if (text_pos != std::string::npos) {
            size_t text_start = json.find("\"", text_pos + 7) + 1;
            size_t text_end = json.find("\"", text_start);
            std::string text = json.substr(text_start, text_end - text_start);
            return std::make_unique<TextPart>(text);
        }
    } else if (kind == "file") {
        // Simplified file parsing
        return std::make_unique<FilePart>("file.dat", "application/octet-stream", std::vector<uint8_t>());
    } else if (kind == "data") {
        size_t data_pos = json.find("\"data\":");
        if (data_pos != std::string::npos) {
            size_t data_start = data_pos + 7;
            size_t brace_count = 0;
            size_t data_end = data_start;
            
            for (size_t i = data_start; i < json.length(); ++i) {
                if (json[i] == '{' || json[i] == '[') brace_count++;
                else if (json[i] == '}' || json[i] == ']') {
                    if (brace_count > 0) brace_count--;
                    else {
                        data_end = i;
                        break;
                    }
                }
            }
            
            std::string data = json.substr(data_start, data_end - data_start);
            return std::make_unique<DataPart>(data);
        }
    }
    
    return nullptr;
}

} // namespace a2a

