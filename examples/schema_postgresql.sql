CREATE TABLE customer_profile (
  email VARCHAR(255) PRIMARY KEY,
  id BIGINT NOT NULL,
  uuid VARCHAR(36) NOT NULL,
  first_name VARCHAR(64) NOT NULL,
  last_name VARCHAR(64) NOT NULL,
  age INT,
  gender VARCHAR(16),
  phone_number VARCHAR(32),
  notes TEXT,
  created_at TIMESTAMP,
  salary NUMERIC(10,2),
  department VARCHAR(64),
  status VARCHAR(32),
  country VARCHAR(64),
  is_active BOOLEAN,
  CONSTRAINT uq_customer_profile_uuid UNIQUE (uuid)
);
