# poco_xml_to_kv
实现ClickHouse的xml配置文件的poco::xml::document对象与kv对象的相互转化。

对kv的定义：

```c++
class Item {
public:
    Item(std::string _name) : name(_name) {}
    Item() {}
    std::map<std::string, std::string> attr; // 参数的属性
    std::string name;  // 配置的参数名
    std::string value;  // 参数值
};
```

xml文件示例：

```xml
<clickhouse>
	<listen_host>0.0.0.0</listen_host>
	<listen_host>localhost</listen_host>

	<global_config>uncompressed_cache_size,remote_servers.shard_0.shard[0]</global_config>
	<logger>
		<level>trace</level>
		<log>/root/chdata/log/clickhouse-server/clickhouse-server.log</log>
		<size>100M</size>
	</logger>

	<uncompressed_cache_size>8589934592</uncompressed_cache_size>
	<mark_cache_size>5368709120</mark_cache_size>
	<mlock_executable>true</mlock_executable>

	<remote_servers>
		<shard_0>
			<shard c1="5">
				<replica>
					<host a1="1" a2="2">clickhouse-env-dev-ch</host>
					<port>9000</port>
				</replica>
				<replica>
					<host a3="3" a4="4">clickhouse-env-dev-ch2</host>
					<port>9000</port>
				</replica>
			</shard>
			<shard>
				<replica>
					<host b1="1" b2="2">clickhouse-env-runner-0.clickhouse-env-runner</host>
					<port>9000</port>
				</replica>
				<replica>
					<host b3="3" b4="4">clickhouse-env-runner-0.clickhouse-env-runner2</host>
					<port>9000</port>
				</replica>
			</shard>
		</shard_0>
		<shard_1>
			<shard>
				<replica>
					<host>clickhouse-env-runner-0.clickhouse-env-runner</host>
					<port>9000</port>
				</replica>
			</shard>
			<shard>
				<replica>
					<host>clickhouse-env-runner-1.clickhouse-env-runner</host>
					<port>9000</port>
				</replica>
			</shard>
		</shard_1>
	</remote_servers>
</clickhouse>
```

示例转成的kv对象：

> name:value  
> ​    attr_name:attr_value

```
listen_host[0]:0.0.0.0
listen_host[1]:localhost
global_config:uncompressed_cache_size,remote_servers.shard_0.shard[0]
logger.level:trace
logger.log:/root/chdata/log/clickhouse-server/clickhouse-server.log
logger.size:100M
uncompressed_cache_size:8589934592
mark_cache_size:5368709120
mlock_executable:true
remote_servers.shard_0.shard[0]:
    c1:5
remote_servers.shard_0.shard[0].replica[0].host:clickhouse-env-dev-ch
    a1:1
    a2:2
remote_servers.shard_0.shard[0].replica[0].port:9000
remote_servers.shard_0.shard[0].replica[1].host:clickhouse-env-dev-ch2
    a3:3
    a4:4
remote_servers.shard_0.shard[0].replica[1].port:9000
remote_servers.shard_0.shard[1].replica[0].host:clickhouse-env-runner-0.clickhouse-env-runner
    b1:1
    b2:2
remote_servers.shard_0.shard[1].replica[0].port:9000
remote_servers.shard_0.shard[1].replica[1].host:clickhouse-env-runner-0.clickhouse-env-runner2
    b3:3
    b4:4
remote_servers.shard_0.shard[1].replica[1].port:9000
remote_servers.shard_1.shard[0].replica.host:clickhouse-env-runner-0.clickhouse-env-runner
remote_servers.shard_1.shard[0].replica.port:9000
remote_servers.shard_1.shard[1].replica.host:clickhouse-env-runner-1.clickhouse-env-runner
remote_servers.shard_1.shard[1].replica.port:9000
```

此处kv对象的name符合 [Poco/Util/XMLConfiguration.h 中对name的格式](https://docs.pocoproject.org/current/Poco.Util.XMLConfiguration.html)，通过 `has` 方法能判断name对应的参数在xml文件中是否存在。

```c++
using ConfigurationPtr = Poco::AutoPtr<Poco::Util::AbstractConfiguration>;
if (configuration->has(name)){
  ...
}
```

通过[Poco::XML::Document的 `getNodeByPath` 方法]((https://docs.pocoproject.org/current/Poco.XML.Node.html#34251))，能得到 Document 对象中对应 path 的 Node 结点。

name 和 path 对应关系举例：

- name = remote_servers.shard_1.shard[0].replica.host
- path = clickhouse/remote_servers/shard_1/shard[0]/replica/host
