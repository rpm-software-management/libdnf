R"**(
BEGIN;


/* add row 'comment' to table 'trans' */
ALTER TABLE
    trans
ADD
    comment TEXT DEFAULT '';


/* set new schema version */
UPDATE
    config
SET
    value = '1.2'
WHERE
    key = 'version';


COMMIT;
)**"
