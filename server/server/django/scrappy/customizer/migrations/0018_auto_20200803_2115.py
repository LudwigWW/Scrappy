# Generated by Django 3.0.7 on 2020-08-04 01:15

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('customizer', '0017_auto_20200803_2113'),
    ]

    operations = [
        migrations.AlterField(
            model_name='scrapobject',
            name='storage_hint',
            field=models.CharField(default='', max_length=5000),
        ),
    ]
