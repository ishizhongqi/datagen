CREATE TABLE customer_profile (
  email VARCHAR2(255) NOT NULL,
  id NUMBER(19) NOT NULL,
  uuid CHAR(36) NOT NULL,
  first_name VARCHAR2(64) NOT NULL,
  last_name VARCHAR2(64) NOT NULL,
  age NUMBER(10),
  gender VARCHAR2(16),
  phone_number VARCHAR2(32),
  notes CLOB,
  created_at TIMESTAMP,
  salary NUMBER(10,2),
  department VARCHAR2(64),
  status VARCHAR2(32),
  country VARCHAR2(64),
  is_active NUMBER(1),
  CONSTRAINT pk_customer_profile PRIMARY KEY (email),
  CONSTRAINT uq_customer_profile_uuid UNIQUE (uuid)
);
