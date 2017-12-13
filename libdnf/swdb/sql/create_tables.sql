R"**(
    CREATE TABLE trans (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        dt_begin INTEGER NOT NULL,      /* (unix timestamp) date and time of transaction begin */
        dt_end INTEGER,                 /* (unix timestamp) date and time of transaction end */
        rpmdb_version_begin TEXT,
        rpmdb_version_end TEXT,
        releasever TEXT NOT NULL,       /* var: $releasever */
        user_id INTEGER NOT NULL,       /* user ID (UID) */
        cmdline TEXT,                   /* recorded command line (program, options, arguments) */
        done INTEGER NOT NULL           /* (bool) 0: not done; 1: done */
    );
    CREATE TABLE repo (
        id INTEGER PRIMARY KEY,
        repoid TEXT NOT NULL            /* repository ID aka 'repoid' */
    );
    CREATE TABLE console_output (
        id INTEGER PRIMARY KEY,
        trans_id INTEGER REFERENCES trans(id),
        file_descriptor INTEGER NOT NULL,
        line TEXT NOT NULL
    );
    CREATE TABLE item (
        id INTEGER PRIMARY KEY,
        item_type INTEGER NOT NULL              /* (enum) 1: rpm, ...*/
    );
    CREATE TABLE trans_item (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        trans_id INTEGER REFERENCES trans(id),
        item_id INTEGER REFERENCES item(id),
        repo_id INTEGER REFERENCES repo(id),
        replaced_by INTEGER REFERENCES trans_item(id),          /* replaced_by is either NULL or points to trans_item in the same transaction */
        action INTEGER NOT NULL,                                /* (enum) */
        reason INTEGER NOT NULL,                                /* (enum) */
        done INTEGER NOT NULL,                                  /* (bool) 0: not done, 1: done */
        CONSTRAINT trans_item_unique_trans_item UNIQUE (trans_id, item_id)
    );
    CREATE TABLE trans_with (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        trans_id INTEGER REFERENCES trans(id),
        item_id INTEGER REFERENCES item(id),
        CONSTRAINT trans_with_unique_trans_item UNIQUE (trans_id, item_id)
    );
    CREATE TABLE rpm (
        item_id INTEGER UNIQUE NOT NULL,
        name TEXT NOT NULL,
        epoch INTEGER NOT NULL,                 /* empty epoch is stored as 0 */
        version TEXT NOT NULL,
        release TEXT NOT NULL,
        arch TEXT NOT NULL,
        checksum_type TEXT NOT NULL,
        checksum_data TEXT NOT NULL,
        FOREIGN KEY(item_id) REFERENCES item(id),
        CONSTRAINT rpm_unique_nevra UNIQUE (name, epoch, version, release, arch)
    );
    CREATE INDEX rpm_name ON rpm(name);
    CREATE INDEX trans_item_item_id ON trans_item(item_id);

    CREATE TABLE config (
        key TEXT PRIMARY KEY,
        value TEXT NOT NULL
    );
    INSERT INTO config VALUES (
        'version',
        '1.1'
    );
)**"
