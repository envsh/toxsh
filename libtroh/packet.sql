--
-- 表的结构 `packet`
--

CREATE TABLE IF NOT EXISTS `packet` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '包ID',
  `pkt` text NOT NULL COMMENT '数据包内容，16进制格式',
  `ctime` int(11) NOT NULL,
  `mtime` int(11) NOT NULL,
  `log` text NOT NULL,
  `flow` varchar(128) NOT NULL COMMENT '数据包流向，C->S 或者 S->C',
  `seq` bigint(20) NOT NULL COMMENT '数据包序列，用于包接收顺序验证，类似TCP协议中的包序列。',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8;

-- 
-- postgresql
--
