#include <map>
#include <string>
#include <stdexcept>

using namespace std;

class HttpRequest {
private:
    map<string, string> requestLine;
    map<string, string> requestHeader;
    string requestBody;
    string rawData;

    void parseRawData();

    static map<string, string> reqLineToMap(const string &reqLine);
    static map<string, string> reqHeaderToMap(const string &reqHeader);

public:
    HttpRequest() {

    }

    // 如果一个构造器是接收单个参数的，那么最好要加上 explicit
    // 如果不加的话，该构造函数还会拥有类型转换的情形，造成混淆
    explicit HttpRequest(string &rawData) : rawData(rawData) {
        // 如果形参是引用的话，不存在参数为 NULL 的情况，即使强制传 NULL，编译器也不通过，因为类型不匹配
        parseRawData();
    }

    string getURL();
    string getMethod();
    string getHttpVersion();
    string getHeader(string key);
    string getBody();

    static bool isAllData(const string &data);
};


/**
 * 请求行字符串转 map
 *
 * 比如："GET / HTTP/1.1" （字符串后面没有 \r\n）
 * 转为键值对：
 *      method = "GET"
 *      URL = "/"
 *      HttpVersion = "HTTP/1.1"
 * @param reqLine 请求行字符串
 * @return 请求 map
 */
map<string, string> HttpRequest::reqLineToMap(const string &reqLine) {
    int pRequestMethod, pRequestURL;
    map<string, string> reqLineMap;

    // 请求方法
    pRequestMethod = reqLine.find(' ');
    if (pRequestMethod == -1)
        throw invalid_argument("request line data can't be resolved to map\n");
    reqLineMap["method"] = reqLine.substr(0, pRequestMethod);

    // 请求 URL
    pRequestURL = reqLine.find(' ', pRequestMethod + 1);
    if (pRequestURL == -1)
        throw invalid_argument("request line data can't be resolved to map\n");
    reqLineMap["url"] = reqLine.substr(pRequestMethod + 1, pRequestURL - pRequestMethod - 1);

    // HTTP 版本
    reqLineMap["version"] = reqLine.substr(pRequestURL + 1, reqLine.length() - pRequestURL - 1);

    return reqLineMap;
}

/**
 * 请求头字符串转 map
 *
 * @param reqHeader
 * @return
 */
map<string, string> HttpRequest::reqHeaderToMap(const string &reqHeader) {
    int pMiddle, pEnd, pStart;
    map<string, string> reqHeaderMap;

    pStart = 0;
    do {
        pMiddle = reqHeader.find(":", pStart);
        if (pMiddle == -1)
            throw invalid_argument("request header data can't be resolved to map");
        pEnd = reqHeader.find("\r\n", pStart);
        string key(reqHeader, pStart, pMiddle - pStart);
        string value;
        if (pEnd != -1)
            value = string(reqHeader, pMiddle + 1, pEnd - pMiddle - 1);
        else
            value = string(reqHeader, pMiddle + 1);
        value.erase(0, value.find_first_not_of(' '));   // 去除前面多余空格
        reqHeaderMap[key] = value;
        pStart = pEnd + 2;  // \r\n 2 个字节
    } while (pEnd != -1);

    return reqHeaderMap;
}


/**
 * 对请求字符串进行分割
 */
void HttpRequest::parseRawData() {
    int headerEndPos, pStart, pEnd;
    headerEndPos = rawData.find("\r\n\r\n");   // 找到请求头结束位置
    if (headerEndPos == -1)
        throw invalid_argument("request raw data is not legal\n");

    // 提取请求行
    pStart = 0;
    pEnd = rawData.find("\r\n");
    string reqLine(rawData, 0, pEnd - pStart);
    requestLine = HttpRequest::reqLineToMap(reqLine);    // 请求行转键值对


    // 提取请求头
    pStart = pEnd + 2;  // \r\n 两个字节
    pEnd = rawData.find("\r\n\r\n", pStart);
    if (pEnd != -1) {
        string reqHeader(rawData, pStart, pEnd - pStart);
        requestHeader = HttpRequest::reqHeaderToMap(reqHeader);
    }

    // 提取请求体
    int lineAndHeaderLength = headerEndPos + 4; // \r\n\r\n 占 4 个字节，不要用 sizeof("\r\n\r\n")，用 sizeof 会得到 5
    int rawDataLength = rawData.length();
    if (rawDataLength == lineAndHeaderLength) {
        // 如果数据长度刚好等于在请求头结束位置，那说明没 body
        requestBody = "";
    } else {
        requestBody = rawData.substr(lineAndHeaderLength);
    }
}

/**
 * 判断数据是否收取完毕
 *
 * @param data
 * @return
 */
bool HttpRequest::isAllData(const string &data) {
    // 如果存在 \r\n\r\n，说明请求行和请求头已经发送过来了
    if (data.find("\r\n\r\n") != -1) {
        int pStart = data.find("Content-Length");
        if (pStart != -1) {
            // 提取 Content-Length 对应数值
            pStart = data.find(":", pStart) + 1;
            int pEnd = data.find("\r\n", pStart);
            string sLength(data, pStart, pEnd - pStart);
            int length = stod(sLength); // 尝试转为整数
            pStart = data.find("\r\n\r\n") + 4;
            string body(data, pStart);
            if (body.length() == length)
                return true;
            else
                return false;

        } else {
            return true;
        }
    }
    return false;
}

string HttpRequest::getURL() {
    return requestLine["url"];
}

string HttpRequest::getMethod() {
    return requestLine["method"];
}

string HttpRequest::getHttpVersion() {
    return requestLine["version"];
}


/**
 * 获取请求头 value
 *
 * @param key 请求头的 key
 * @return 请求头的 value
 */
string HttpRequest::getHeader(string key) {
    if (requestHeader.find(key) != requestHeader.end())
        return requestHeader[key];
    else
        return "";
}

/**
 * 获取请求体
 */
string HttpRequest::getBody() {
    return requestBody;
}