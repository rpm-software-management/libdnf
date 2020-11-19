R"**(
BEGIN;

    /* item: module_stream - to track active (installed), inactive (removed) streams */
    CREATE TABLE module_stream (
        item_id INTEGER UNIQUE NOT NULL,
        name TEXT NOT NULL,
        stream TEXT NOT NULL,
        /*
        context TEXT NOT NULL,
        */
        FOREIGN KEY(item_id) REFERENCES item(id)
        /*
        CONSTRAINT module_stream_unique_ns UNIQUE (name, stream)
        */
    );


    /* increase minor version of the schema */
    UPDATE
        config
    SET
        value = '1.3'
    WHERE
        key = 'version';

COMMIT;
)**"
