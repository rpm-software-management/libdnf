R"***(
    CREATE TABLE trans (
        id INTEGER PRIMARY KEY,
        dt_begin INTEGER NOT NULL,
        dt_end INTEGER,
        rpmdb_version_begin TEXT,
        rpmdb_version_end TEXT,
        releasever TEXT NOT NULL,
        cmdline TEXT,
        unix_uid INTEGER,
        return_code INTEGER
    );
    CREATE TABLE repo (
        id INTEGER PRIMARY KEY,
        name TEXT NOT NULL
    );
    CREATE TABLE console_output (
        id INTEGER PRIMARY KEY,
        trans_id INTEGER REFERENCES trans(id),
        file_descriptor INTEGER,
        line TEXT NOT NULL
    );
    CREATE TABLE item_state (
        id INTEGER PRIMARY KEY,
        name TEXT NOT NULL
    );
    CREATE TABLE item (
        id INTEGER PRIMARY KEY,
        item_type INTEGER NOT NULL
    );
    CREATE TABLE trans_item (
        id INTEGER PRIMARY KEY,
        trans_id INTEGER REFERENCES trans(id),
        item_id INTEGER REFERENCES item(id),
        repo_id INTEGER REFERENCES repo(id),
        done INTEGER NOT NULL,
        obsoleting INTEGER NOT NULL,
        reason INTEGER NOT NULL,
        state INTEGER REFERENCES item_state(id)
    );
    CREATE TABLE trans_with (
        id INTEGER PRIMARY KEY,
        trans_id INTEGER REFERENCES trans(id),
        item_id INTEGER REFERENCES item(id)
    );
    CREATE TABLE rpm (
        item_id INTEGER UNIQUE,
        name TEXT NOT NULL,
        epoch INTEGER NOT NULL,
        version TEXT NOT NULL,
        release TEXT NOT NULL,
        arch TEXT NOT NULL,
        checksum_type TEXT NOT NULL,
        checksum_data TEXT NOT NULL,
        FOREIGN KEY(item_id) REFERENCES item(id)
    );
    CREATE INDEX rpm_name ON rpm(name);
    CREATE INDEX trans_item_item_id ON trans_item(item_id);

    CREATE TABLE config (
        key TEXT PRIMERY KEY,
        value TEXT NOT NULL
    );
    INSERT INTO config VALUES (
        'version',
        '1.1'
    );
)***"
