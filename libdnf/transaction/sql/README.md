# SQLite tables
### Init
Init file of database tables is defined in `create_tables.sql`

### Migration
Migration files should always be created following the version number in `config` table `version` col

Name should be according to table version:
if version is 1.2 name should be `migrate_tables_1_2.sql`
