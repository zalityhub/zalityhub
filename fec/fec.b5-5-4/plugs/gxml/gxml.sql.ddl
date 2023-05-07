-- DepositRequest batch header table
--    contains all XML tags needed for the DepositResponse XML Document
--    relates to detail via column 'dep_id'
CREATE TABLE header(
	dep_id INTEGER PRIMARY KEY AUTOINCREMENT,
	msg_id INTEGER(8),
	chain TEXT,
	location TEXT,
	venue TEXT,
	batch_date INTEGER(4),
	batch_nbr INTEGER(4),
	batch_seq INTEGER(4),
	status TEXT DEFAULT 'OK ',
	requests INTEGER(4) DEFAULT 0,
	responses INTEGER(4) DEFAULT 0,
	at_eof INTEGER(1) DEFAULT 0,
	xml_length INTEGER(4) DEFAULT 0,
	UNIQUE(batch_date,batch_nbr,batch_seq,chain,location,venue)
);

-- DepositRequest batch header table unique index
--    speeds up Host response SELECT
--    speeds up UPDATE trigger access
CREATE UNIQUE INDEX hdr_idx ON header (
	msg_id,
	batch_date,
	batch_nbr,
	batch_seq,
	chain,
	location,
	venue
);

-- DepositRequest batch detail table
--    contains all XML tags needed for the DepositResponse 'Record' tag(s)
--    relates to header via column 'dep_id'
--    'xml_resp' contains the formatted 'Record' XML object response tags
CREATE TABLE detail (
	dep_id INTEGER,
	record_nbr INTEGER(4),
	xml_resp TEXT,
	UNIQUE(dep_id,record_nbr)
);

-- DepositRequest batch detail table unique index
--    speeds up Host response UPDATE
CREATE UNIQUE INDEX dtl_idx ON detail (
	dep_id,
	record_nbr
);

-- DepositRequest batch detail table trigger
--    keep count of detail Record(s) in related header row
--    ORDER BY DEFAULTs to header.dep_id PRIMARY KEY
CREATE TRIGGER count_request AFTER INSERT ON detail
BEGIN
    UPDATE header SET requests = 1 + requests WHERE dep_id = new.dep_id;
END;

-- DepositRequest batch detail table trigger
--    keep count of detail Record(s) in related header row
--    keeps running total of the detail 'Record' XML response tags
--    ORDER BY DEFAULTs to header.dep_id PRIMARY KEY
CREATE TRIGGER count_response AFTER UPDATE ON detail
BEGIN
    UPDATE header SET responses = 1 + responses,
	   xml_length = length(new.xml_resp) + xml_length
	   WHERE dep_id = new.dep_id;
END;

-- DepositRequest batch header table trigger
--    removes all batch detail table rows aftere header row DELETE
CREATE TRIGGER delete_deposit AFTER DELETE ON header
BEGIN
    DELETE FROM detail WHERE dep_id = old.dep_id;
END;

