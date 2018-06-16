import os
from datetime import timedelta
DEBUG = True

dialect = 'mysql'
driver = 'pymysql'
username = 'root'
password = '68700695a'
port = '3306'
database = 'Q_A'

SQLALCHEMY_DATABASE_URI = '{}+{}://{}:{}@localhost:{}/{}?charset=utf8'.format(dialect,driver,username,password,port,database)
SQLALCHEMY_TRACK_MODIFICATIONS = False

SECRET_KEY = os.urandom(24)

PERMANENT_SESSION_LIFETIME = timedelta(days=7)