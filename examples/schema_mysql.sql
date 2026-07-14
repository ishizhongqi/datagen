CREATE TABLE customer_profile (
  email VARCHAR(255) NOT NULL,
  id BIGINT NOT NULL,
  uuid CHAR(36) NOT NULL,
  first_name VARCHAR(64) NOT NULL,
  last_name VARCHAR(64) NOT NULL,
  age INT,
  gender VARCHAR(16),
  phone_number VARCHAR(32),
  notes TEXT,
  created_at DATETIME,
  salary DECIMAL(10,2),
  department VARCHAR(64),
  status VARCHAR(32),
  country VARCHAR(64),
  is_active TINYINT(1),
  PRIMARY KEY (email),
  UNIQUE KEY uq_customer_profile_uuid (uuid)
);
