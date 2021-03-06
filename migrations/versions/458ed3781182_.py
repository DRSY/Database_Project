"""empty message

Revision ID: 458ed3781182
Revises: 6b00c44646e3
Create Date: 2017-11-27 17:20:42.210108

"""
from alembic import op
import sqlalchemy as sa


# revision identifiers, used by Alembic.
revision = '458ed3781182'
down_revision = '6b00c44646e3'
branch_labels = None
depends_on = None


def upgrade():
    # ### commands auto generated by Alembic - please adjust! ###
    op.create_table('comment',
    sa.Column('id', sa.Integer(), nullable=False),
    sa.Column('content', sa.Text(), nullable=False),
    sa.Column('itemid', sa.Integer(), nullable=True),
    sa.Column('ownerid', sa.Integer(), nullable=True),
    sa.ForeignKeyConstraint(['itemid'], ['item.id'], ),
    sa.ForeignKeyConstraint(['ownerid'], ['user.id'], ),
    sa.PrimaryKeyConstraint('id')
    )
    # ### end Alembic commands ###


def downgrade():
    # ### commands auto generated by Alembic - please adjust! ###
    op.drop_table('comment')
    # ### end Alembic commands ###
