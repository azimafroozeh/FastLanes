# FastLanes Supported Types

Fastlanes accepts a broad range of SQL‑like type names. The table below lists **only the SQL type names** recognised by the fastlanes together with a short description of the data they represent.

| SQL Type (normalised)    | Description                                          |
| ------------------------ | ---------------------------------------------------- |
| TINYINT                  | 8‑bit signed integer                                 |
| SMALLINT                 | 16‑bit signed integer                                |
| INT / INTEGER            | 32‑bit signed integer (or bigger, dialect‑dependent) |
| BIGINT                   | 64‑bit signed integer                                |
| TINYINT UNSIGNED         | 8‑bit unsigned integer                               |
| UTINYINT / UINT8         | 8‑bit unsigned integer (DuckDB alias)                |
| SMALLINT UNSIGNED        | 16‑bit unsigned integer                              |
| USMALLINT / UINT16       | 16‑bit unsigned integer (DuckDB alias)               |
| MEDIUMINT                | 24‑bit signed integer (MySQL)                        |
| MEDIUMINT UNSIGNED       | 24‑bit unsigned integer (MySQL)                      |
| INT UNSIGNED / UINT32    | 32‑bit unsigned integer                              |
| BIGINT UNSIGNED / UINT64 | 64‑bit unsigned integer                              |
| FLOAT                    | 32‑bit IEEE‑754 floating point                       |
| DOUBLE                   | 64‑bit IEEE‑754 floating point                       |
| DECIMAL(p,s)             | Fixed‑precision numeric with scale `s`               |
| BOOLEAN / BOOL           | Boolean (true / false)                               |
| BIT (BIT(1))             | Single‑bit boolean                                   |
| BIT VARYING              | Variable‑length bit string                           |
| CHAR(n) / VARCHAR(n)     | Fixed / variable‑length character string             |
| TINYTEXT / TEXT          | Variable‑length text (up to 2³¹‑1 bytes)             |
| MEDIUMTEXT / LONGTEXT    | Variable‑length text (larger limits)                 |
| ENUM / SET / JSON        | String‑encoded values                                |
| UUID                     | 128‑bit universally unique identifier                |
| TINYBLOB / BLOB          | Variable‑length binary blob                          |
| MEDIUMBLOB / LONGBLOB    | Variable‑length binary blob (larger limits)          |
| DATE                     | Calendar date (YYYY‑MM‑DD)                           |
| DATETIME / TIMESTAMP     | Date‑time (microsecond precision)                    |
| TIME                     | Time of day or interval                              |
| YEAR                     | 4‑digit year value                                   |
| INTERVAL                 | SQL interval literal                                 |
| LIST                     | Ordered collection (array)                           |
| STRUCT                   | Composite record with named fields                   |
| MAP                      | Key‑value collection                                 |
| BYTE\_ARRAY              | Raw binary data                                      |
| JPEG                     | JPEG‑encoded binary image                            |
