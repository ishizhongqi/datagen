CREATE TABLE customer_profile (
  email TEXT NOT NULL PRIMARY KEY,
  id INTEGER NOT NULL,
  uuid TEXT NOT NULL,
  first_name TEXT NOT NULL,
  last_name TEXT NOT NULL,
  age INTEGER,
  gender TEXT,
  phone_number TEXT,
  notes TEXT,
  created_at DATETIME,
  salary DECIMAL(10,2),
  department TEXT,
  status TEXT,
  country TEXT,
  is_active INTEGER,
  CONSTRAINT uq_customer_profile_uuid UNIQUE (uuid)
);
