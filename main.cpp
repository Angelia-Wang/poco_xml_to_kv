#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>

#include <Poco/AutoPtr.h> //Poco::AutoPtr
#include <Poco/DOM/Document.h> // Poco::XML::Document
#include <Poco/DOM/Element.h>  // Poco::XML::Element
#include <Poco/DOM/Text.h>       // Poco::XML::Text
#include <Poco/DOM/CDATASection.h>    // Poco::XML::CDATASection
#include <Poco/DOM/ProcessingInstruction.h> // Poco::XML::ProcessingInstruction
#include <Poco/DOM/Comment.h>  // Poco::XML::Comment
#include <Poco/DOM/DOMWriter.h> // Poco::XML::DOMWriter
#include <Poco/XML/XMLWriter.h> // Poco::XML::XMLWriter

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Node.h>
#include <Poco/DOM/NamedNodeMap.h>
#include <Poco/XML/XMLString.h>
#include <Poco/XML/XMLException.h>
#include <Poco/XML/XMLStream.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/NodeFilter.h>

#include <Poco/Util/AbstractConfiguration.h>
#include <Poco/Util/XMLConfiguration.h>

using namespace Poco::XML;
using ConfigurationPtr = Poco::AutoPtr<Poco::Util::AbstractConfiguration>;
using XMLDocumentPtr = Poco::AutoPtr<Poco::XML::Document>;
using NamedNodeMapPtr = Poco::AutoPtr<Poco::XML::NamedNodeMap>;
using NodeListPtr = Poco::AutoPtr<Poco::XML::NodeList>;

class Item {
public:
    Item(std::string _name) : name(_name) {}
    Item() {}
    std::map<std::string, std::string> attr; // 参数的属性
    std::string name;  // 配置的参数名
    std::string value;  // 参数值
};

using KV = Item;
using KVs = std::vector<KV>;

/// 打印kv信息
void printNodeInfo(KV item) {
    std::cout << item.name << ":" << item.value << std::endl;
    if (!item.attr.empty()) {
        for (auto it = item.attr.begin(); it != item.attr.end(); it++) {
            std::cout << "    " << it->first << ":" << it->second << std::endl;
        }
    }
}


/**
 * 读取xml文件到内存
 * @return
 */
XMLDocumentPtr read_xml() {
    DOMParser parser;
    XMLDocumentPtr config = parser.parse("../example.xml");
    return config;
}

/**
 * 输出xml文件
 * @param doc
 */
void write_xml(XMLDocumentPtr doc) {
    Poco::XML::DOMWriter writer;

    //writer.setOptions(Poco::XML::XMLWriter::CANONICAL);
    //writer.setOptions(Poco::XML::XMLWriter::PRETTY_PRINT_ATTRIBUTES); //
    //writer.setOptions(Poco::XML::XMLWriter::CANONICAL_XML);
    //writer.setOptions(Poco::XML::XMLWriter::WRITE_XML_DECLARATION);// add <?xml version='1.0' encoding='UTF-8'?>
    writer.setOptions(Poco::XML::XMLWriter::PRETTY_PRINT); // 每一个元素都在文件中另起一行，易于读者观看

    std::fstream ofs("test.xml", std::ios::in);
    writer.writeNode(ofs, doc);

    std::stringstream sstr;
    writer.writeNode(sstr, doc);
    std::string s = sstr.str();
    std::cout << s << std::endl;
}

/**
 * 获取文档的根结点
 * @param document
 * @return
 */
static Node *getRootNode(Document *document) {
    const NodeListPtr children = document->childNodes();
    for (size_t i = 0, size = children->length(); i < size; ++i) {
        Node *child = children->item(i);
        /// Besides the root element there can be comment nodes on the top level.
        /// Skip them.
        if (child->nodeType() == Node::ELEMENT_NODE)
            return child;
    }

    throw Poco::Exception("No root node in document");
}

/**
 * 将poco::xml::document对象转化为细粒度kv的vector
 * @param root document对象的root结点
 * @param prefix 前缀
 * @param kvs 细粒度kv的vector
 * @param flag 标记当前结点是否存在同名的相邻结点
 * @param index 若存在相邻结点，需要index进行标号
 */
static void transformXMLToKV(const Node *root, std::string prefix, KVs &kvs, bool flag, size_t index = -1) {
    if (root->nodeType() == Node::TEXT_NODE) {
        // 是内容，到达终点
        if (root->getNodeValue().find_first_of('\n') == std::string::npos) {
            KV temp;
            temp.name = prefix;
            temp.value = root->getNodeValue();
            if (root->parentNode()->hasAttributes()) {
                NamedNodeMapPtr map = root->parentNode()->attributes(); // 获取属性
                for (unsigned long i = 0; i < map->length(); i++) {
                    Node *attribute = map->item(i);
                    temp.attr[attribute->nodeName()] = attribute->nodeValue();
                }
            }
            printNodeInfo(temp);  // 输出temp信息
            kvs.push_back(temp);
        }
            // 不是内容，但若有属性，也存入Item
        else if (root->parentNode()->hasAttributes() && root == root->parentNode()->firstChild()) {
            KV temp;
            temp.name = prefix;
            NamedNodeMapPtr map = root->parentNode()->attributes(); // 获取属性
            for (unsigned long i = 0; i < map->length(); i++) {
                Node *attribute = map->item(i);
                temp.attr[attribute->nodeName()] = attribute->nodeValue();
            }
            printNodeInfo(temp);  // 输出temp信息
            kvs.push_back(temp);
        }
    }

    // 1. 更新前缀
    std::string root_name = root->nodeName();
    bool cur_flag = false; // 标记当前结点是否要加入前缀
    if (root->nodeType() == Node::ELEMENT_NODE && root_name != "clickhouse")
        cur_flag = true;
    // 添加当前结点的标签作为前缀
    if (cur_flag) {
        if (prefix.empty())
            prefix += root_name;
        else
            prefix = prefix + '.' + root_name; // 前缀
    }
    // 若存在参数名相同的相邻结点，则通过[$num]区分
    if (flag) {
        prefix = prefix + '[' + std::to_string(index) + ']';
    }
    // 2. 遍历子结点
    const NodeListPtr children = root->childNodes();
    for (size_t i = 0, size = children->length(), pos = -1; i < size; ++i) {
        Node *child = children->item(i);
        bool child_flag = false;
        if (child->nodeType() == Node::ELEMENT_NODE) {
            // 该子结点的后面的兄弟结点是否有同名
            if (i + 2 < size) {
                if (child->nodeName() == child->nextSibling()->nextSibling()->nodeName()) {
                    child_flag = true;
                    pos++;
                }
            }
            // 该子结点的前面的兄弟结点是否有同名
            if (i >= 2) {
                if (child->nodeName() == child->previousSibling()->previousSibling()->nodeName()) {
                    child_flag = true;
                    pos++;
                }
            }
        }
        transformXMLToKV(child, prefix, kvs, child_flag, pos);
    }
    // 还原标签
    if (flag) {
        prefix.erase(prefix.find_last_of('['), 3);
        flag = false;
    }
    if (cur_flag) {
        std::string::size_type pos = prefix.find_last_of('.');
        if (pos != std::string::npos)
            prefix.erase(pos, root_name.size());
        cur_flag = false;
    }
}

/**
 * 从 Node结点的 name 获取 path
 * @param name 结点name，如 remote_servers.shard_1.shard[0].replica.host
 * @return 结点的path，如 clickhouse/remote_servers/shard_1/shard[0]/replica/host
 */
std::string getPathFromName(std::string name) {
    std::string::size_type pos;
    while ((pos = name.find('.')) != std::string::npos) {
        name.replace(pos, 1, "/");
    }
    return "clickhouse/" + name;
}

/**
 * 获得结点，若结点不存在则创建
 * @param doc poco::xml::document对象
 * @param configuration Poco::Util::AbstractConfiguration对象
 * @param item 结点对应的KV
 * @param name 结点的名称（参数名），如 remote_servers.shard_1.shard[0].replica.host
 * @param path 结点的路径，如 clickhouse/remote_servers/shard_1/shard[0]/replica/host
 * @return 结点
 */
static Node * createNode(XMLDocumentPtr doc, ConfigurationPtr configuration, KV *item, std::string name, std::string path) {
    // 若当前结点不存在
    if (!configuration->has(name)) {
        // 当前结点的参数名（不包含前缀）
        std::string::size_type pos = path.find_last_of('/');
        std::string curr_node_name = path.substr(pos + 1, path.size() - pos - 1);
        std::string parent_path = path.substr(0, pos);
        std::string parent_name;

        pos = curr_node_name.find('[');
        if (pos != std::string::npos)
            curr_node_name.erase(pos, 3);

        // 当前结点的父结点的参数名（包含前缀）
        pos = name.find_last_of('.');
        if (pos != std::string::npos)
            parent_name = name.substr(0, pos);

        Node *parent;
        // 1. 若父结点存在，则找到父结点，否则递归创建父结点
        if (configuration->has(parent_name)) {
            parent = doc->getNodeByPath(parent_path);
        } else {
            KV parent_item = KV(parent_name);
            parent = createNode(doc, configuration, &parent_item, parent_name, parent_path);
        }
        // 2. 创建子结点
        Poco::AutoPtr<Poco::XML::Element> temp = doc->createElement(curr_node_name);
        // 子结点属性
        if (!item->attr.empty()) {
            for (auto &it: item->attr) {
                temp->setAttribute(it.first, it.second);
            }
        }
        // 子结点值
        if (!item->value.empty()) {
            Poco::AutoPtr<Poco::XML::Text> txt = doc->createTextNode(item->value);
            temp->appendChild(txt);
        }
        parent->appendChild(temp);
        return temp;
    }
        // 若当前结点已存在，判断属性是否完全
    else {
        Element *cur = dynamic_cast<Element *>(doc->getNodeByPath(path));
        if (!cur->hasAttributes() && !item->attr.empty()) {
            for (auto &it: item->attr) {
                cur->setAttribute(it.first, it.second);
            }
        }
        return cur;
    }
}

/**
 * 将一系列kv转为xml子树
 * @param kv
 * @return XMLDocumentPtr
 */
XMLDocumentPtr transformKVToXML(const KVs &kvs) {
    XMLDocumentPtr config = new Poco::XML::Document;
    Poco::AutoPtr<Poco::XML::Element> root = config->createElement("clickhouse");
    config->appendChild(root);
    ConfigurationPtr configuration(new Poco::Util::XMLConfiguration(config));
    std::string path;
    for (const auto &kv: kvs) {
        KV temp = kv;
        path = getPathFromName(kv.name);
        Node *tt = createNode(config, configuration, &temp, kv.name, path);
    }
    return config;
}



int main(int argc, char *argv[])
{
    XMLDocumentPtr config = read_xml();
    Node *node = getRootNode(config);
    KVs kvs;
    transformXMLToKV(node, "", kvs, false);
    config = transformKVToXML(kvs);
    write_xml(config);
    return 0;
}

