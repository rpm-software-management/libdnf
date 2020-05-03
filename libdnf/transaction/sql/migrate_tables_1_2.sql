R"**(
BEGIN TRANSACTION;
    ALTER TABLE trans
        ADD comment TEXT DEFAULT '';
    UPDATE config
        SET value = '1.2'
        WHERE key = 'version';
COMMIT;
)**"
