R"**(
BEGIN TRANSACTION;
    ALTER TABLE trans
        ADD persistence INTEGER DEFAULT 0;
    UPDATE config
        SET value = '1.3'
        WHERE key = 'version';
COMMIT;
)**"
