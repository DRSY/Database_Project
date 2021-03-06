"""empty message

Revision ID: 45ada54467e4
Revises: 2da5ffc35cb0
Create Date: 2017-12-02 13:47:29.989644

"""
from alembic import op
import sqlalchemy as sa


# revision identifiers, used by Alembic.
revision = '45ada54467e4'
down_revision = '2da5ffc35cb0'
branch_labels = None
depends_on = None


def upgrade():
    # ### commands auto generated by Alembic - please adjust! ###
    op.create_table('interest',
    sa.Column('id', sa.Integer(), nullable=False),
    sa.Column('userid', sa.Integer(), nullable=True),
    sa.Column('itemid', sa.Integer(), nullable=True),
    sa.ForeignKeyConstraint(['itemid'], ['item.id'], ),
    sa.ForeignKeyConstraint(['userid'], ['user.id'], ),
    sa.PrimaryKeyConstraint('id')
    )
    # ### end Alembic commands ###


def downgrade():
    # ### commands auto generated by Alembic - please adjust! ###
    op.drop_table('interest')
    # ### end Alembic commands ###
